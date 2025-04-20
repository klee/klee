//===-- RLInterface.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RLInterface.h"
#include "ExecutionState.h"
#include "ExecutionTree.h"

#include "klee/Statistics/Statistics.h"
#include "klee/Module/KInstruction.h"

#include <random>

using namespace klee;

// Policy network implementation
RLInterface::PolicyNetwork::PolicyNetwork(size_t input_size, size_t hidden_size) {
  fc1 = register_module("fc1", torch::nn::Linear(input_size, hidden_size));
  fc2 = register_module("fc2", torch::nn::Linear(hidden_size, 1));
}

torch::Tensor RLInterface::PolicyNetwork::forward(torch::Tensor x) {
  x = torch::relu(fc1->forward(x));
  x = torch::sigmoid(fc2->forward(x));
  return x;
}

RLInterface::RLInterface() 
  : policy(std::make_shared<PolicyNetwork>(10, 64)), // 10 features, 64 hidden units
    optimizer(policy->parameters(), torch::optim::AdamOptions(learning_rate)) {
  // Initialize the policy network with default parameters
}

torch::Tensor RLInterface::extractFeatures(ExecutionTreeNode* node) {
  // Extract relevant features from the execution tree node
  std::vector<float> features;
  
  // If node is null, return zeros
  if (!node) {
    return torch::zeros({10});
  }
  
  // Extract features like:
  // 1. Depth in the tree
  features.push_back(static_cast<float>(node->depth));
  
  // 2. Has state or not
  features.push_back(node->state ? 1.0f : 0.0f);
  
  // 3. If it has a state, extract state features
  if (node->state) {
    // Instructions executed
    features.push_back(static_cast<float>(node->state->executedInstructions));
    
    // Constraints size
    features.push_back(static_cast<float>(node->state->constraints.size()));
    
    // Stack size
    features.push_back(static_cast<float>(node->state->stack.size()));
    
    // Query cost
    features.push_back(static_cast<float>(node->state->queryMetaData.queryCost.toSeconds()));
  } else {
    // Fill with zeros if no state
    features.push_back(0.0f);
    features.push_back(0.0f);
    features.push_back(0.0f);
    features.push_back(0.0f);
  }
  
  // 4. Has children or not
  features.push_back(node->left.getPointer() ? 1.0f : 0.0f);
  features.push_back(node->right.getPointer() ? 1.0f : 0.0f);
  
  // 5. Parent information
  features.push_back(node->parent ? 1.0f : 0.0f);
  
  // Pad to 10 features if needed
  while (features.size() < 10) {
    features.push_back(0.0f);
  }
  
  return torch::tensor(features).unsqueeze(0);
}

bool RLInterface::chooseAction(ExecutionTreeNode* left, ExecutionTreeNode* right) {
  // Epsilon-greedy exploration
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0, 1.0);
  
  if (dis(gen) < epsilon) {
    // Random action
    return dis(gen) < 0.5;
  }
  
  // Extract features for both nodes
  torch::Tensor leftFeatures = extractFeatures(left);
  torch::Tensor rightFeatures = extractFeatures(right);
  
  // Get policy predictions
  torch::NoGradGuard no_grad;
  float leftValue = policy->forward(leftFeatures).item<float>();
  float rightValue = policy->forward(rightFeatures).item<float>();
  
  // Choose the action with higher value (true = left, false = right)
  return leftValue > rightValue;
}

void RLInterface::addExperience(ExecutionTreeNode* state, bool action, float reward, 
                               ExecutionTreeNode* nextState, bool done) {
  // Extract features
  torch::Tensor stateFeatures = extractFeatures(state);
  torch::Tensor nextStateFeatures = extractFeatures(nextState);
  
  // Add to replay buffer
  replayBuffer.push_back({stateFeatures, action ? 1 : 0, reward, nextStateFeatures, done});
  
  // Limit buffer size
  if (replayBuffer.size() > replayBufferSize) {
    replayBuffer.erase(replayBuffer.begin());
  }
  
  // Train if buffer has enough samples
  if (replayBuffer.size() >= batchSize) {
    trainStep();
  }
}

void RLInterface::trainStep() {
  // Skip if not enough experiences
  if (replayBuffer.size() < batchSize) {
    return;
  }
  
  // Sample batch
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, replayBuffer.size() - 1);
  
  std::vector<Experience> batch;
  for (size_t i = 0; i < batchSize; ++i) {
    batch.push_back(replayBuffer[dis(gen)]);
  }
  
  // Prepare tensors
  std::vector<torch::Tensor> states, next_states;
  std::vector<int> actions;
  std::vector<float> rewards;
  std::vector<bool> dones;
  
  for (const auto& exp : batch) {
    states.push_back(exp.state);
    actions.push_back(exp.action);
    rewards.push_back(exp.reward);
    next_states.push_back(exp.next_state);
    dones.push_back(exp.done);
  }
  
  torch::Tensor stateBatch = torch::cat(states);
  torch::Tensor actionBatch = torch::tensor(actions);
  torch::Tensor rewardBatch = torch::tensor(rewards);
  torch::Tensor nextStateBatch = torch::cat(next_states);
  torch::Tensor doneBatch = torch::tensor(dones).to(torch::kFloat);
  
  // Compute target Q values
  torch::Tensor nextValues;
  {
    torch::NoGradGuard no_grad;
    nextValues = policy->forward(nextStateBatch);
  }
  
  torch::Tensor targetValues = rewardBatch + discount_factor * nextValues.squeeze() * (1 - doneBatch);
  
  // Compute current Q values
  torch::Tensor currentValues = policy->forward(stateBatch).squeeze();
  
  // Compute loss
  torch::Tensor loss = torch::mse_loss(currentValues, targetValues);
  
  // Optimize
  optimizer.zero_grad();
  loss.backward();
  optimizer.step();
}

void RLInterface::saveModel(const std::string& path) {
  torch::save(policy, path);
}

void RLInterface::loadModel(const std::string& path) {
  torch::load(policy, path);
}
