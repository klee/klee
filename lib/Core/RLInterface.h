//===-- RLInterface.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_RLINTERFACE_H
#define KLEE_RLINTERFACE_H

#include <torch/torch.h>
#include <vector>
#include <memory>

namespace klee {

class ExecutionState;
class ExecutionTreeNode;

/// RLInterface provides a reinforcement learning interface for KLEE path selection.
/// It uses libtorch to implement a simple policy network to decide which paths to explore.
class RLInterface {
private:
  // Policy network
  struct PolicyNetwork : torch::nn::Module {
    PolicyNetwork(size_t input_size, size_t hidden_size);

    torch::nn::Linear fc1{nullptr}, fc2{nullptr};
    torch::Tensor forward(torch::Tensor x);
  };
  
  std::shared_ptr<PolicyNetwork> policy;
  torch::optim::Adam optimizer;
  
  // Hyperparameters
  float learning_rate{0.001};
  float discount_factor{0.99};
  float epsilon{0.1}; // For epsilon-greedy exploration
  
  // Feature extraction
  torch::Tensor extractFeatures(ExecutionTreeNode* node);
  
  // Experience replay buffer
  struct Experience {
    torch::Tensor state;
    int action;
    float reward;
    torch::Tensor next_state;
    bool done;
  };
  
  std::vector<Experience> replayBuffer;
  size_t replayBufferSize{10000};
  size_t batchSize{32};
  
  // Training
  void trainStep();
  
public:
  RLInterface();
  ~RLInterface() = default;
  
  // Choose an action based on the current state
  bool chooseAction(ExecutionTreeNode* left, ExecutionTreeNode* right);
  
  // Add experience to replay buffer
  void addExperience(ExecutionTreeNode* state, bool action, float reward, 
                     ExecutionTreeNode* nextState, bool done);
  
  // Save and load model
  void saveModel(const std::string& path);
  void loadModel(const std::string& path);
};

} // namespace klee

#endif /* KLEE_RLINTERFACE_H */
