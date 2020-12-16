Thank you for contributing to KLEE.  We are looking forward to reviewing your PR.  However, given the small number of active reviewers and our limited time, it might take a while to do so.  We aim to get back to each PR within one month, and often do so within one week. 

To help expedite the review please ensure the following, by adding an "x" for each completed item:

- [ ] The PR addresses a single issue.  In other words, if some parts of a PR could form another independent PR, you should break this PR into multiple smaller PRs.
- [ ] There are no unnecessary commits. For instance, commits that fix issues with a previous commit in this PR are unnecessary and should be removed (you can find [documentation on squashing commits here](https://github.com/edx/edx-platform/wiki/How-to-Rebase-a-Pull-Request#squash-your-changes)).
- [ ] Larger PRs are divided into a logical sequence of commits.
- [ ] Each commit has a meaningful message documenting what it does.
- [ ] The code is commented.  In particular, newly added classes and functions should be documented.
- [ ] The patch is formatted via  [clang-format](https://clang.llvm.org/docs/ClangFormat.html) (see also [git-clang-format](https://raw.githubusercontent.com/llvm/llvm-project/master/clang/tools/clang-format/git-clang-format) for Git integration).  Please only format the patch itself and code surrounding the patch, not entire files.  Divergences from clang-formatting are only rarely accepted, and only if they clearly improve code readability.
- [ ] Add test cases exercising the code you added or modified.  We expect [system and/or unit test cases](https://klee.github.io/docs/developers-guide/#regression-testing-framework) for all non-trivial changes.  After you submit your PR, you will be able to see a [Codecov report](https://docs.codecov.io/docs/pull-request-comments) telling you which parts of your patch are not covered by the regression test suite.  You will also be able to see if the Github Actions CI and Cirrus CI tests have passed.  If they don't, you should examine the failures and address them before the PR can be reviewed. 
- [ ] Spellcheck all messages added to the codebase, all comments, as well as commit messages.
