## SubrosaDG Project Specification
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FSubrosaDG-dev%2FSubrosaDG.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FSubrosaDG-dev%2FSubrosaDG?ref=badge_shield)


### git collaborate

1. The way to clone this project is `git clone git@github.com:SubrosaDG-dev/SubrosaDG.git` .

2. The branch for the server-side project is master . It is recommended to create a local dev branch after cloning (this branch does not need to be uploaded). Most of the commits should be made on the local dev branch. The commits on the dev branch can be made freely, and they can be merged into the local master branch after completing certain modules.

3. You can switch to the master branch and use `git pull` to fetch the remote code.

4. It is recommended to fetch the remote master branch and update it before merging the local dev branch into the local master branch to avoid conflicts when pushing the local master branch to the remote master branch.

5. When merging the dev branch into the master branch, it is recommended to use `git merge --squash dev` . Adding the `--squash` option here is to unify the series of commits generated on the local dev branch into a single commit before merging them into the master branch.

6. After merging the dev branch into the master branch and pushing it to the remote master branch, the dev branch will be one commit behind the master branch.
```bash
... o ---- A ---- B ---- C  origin/master (upstream work)
            \
             C ---- D  dev (your work)
```
If no one has modified the master branch before merging, the content of the master branch and the dev branch will be the same, and no further operation is needed. However, if someone has modified the master branch before merging, there will be differences between the content of the two branches. In this case, the remote master branch needs to be updated, and the local dev branch needs to be updated at the same time. Since the last merge was processed as one commit, `cherry-pick` can be used to repeat the previous commit.
```bash
git cherry-pick ${branch master merge commit id}
```
If there was a conflict resolution before, it needs to be done again. At this point, you can use `git checkout master ${filename}` to use the code in the master branch as a reference. The submission line of git should be:
```bash
... o ---- A ---- B ---- C  origin/master (upstream work)
            \
             C ---- D ---- M dev (your work)
```
This operation is to protect the previous commits on the dev branch. Using other methods may affect the commit history of the dev branch.

7. It is recommended to add the following settings in the setting.json of vscode to avoid uploading the dev branch. The third setting can prevent vscode from searching the upper-level git repository.
```json
"git.showActionButton": {
    "publish": false,
    "sync": false
}
"git.showPushSuccessNotification": true,
"git.autoRepositoryDetection": false,
```

### build toolchain

1. Use cmake to generate build.ninja and compile with clang using ccache for speedup.

2. The project uses vcpkg to import third-party libraries, including libconfig, fmt, spdlog, eigen3, openmp (compiler-provided), openmpi, and dbg-macro. Although vcpkg is integrated into cmake, it is recommended to install the libraries locally first. During the project build, the compilation cache will be copied directly. Some components depend on each other, and vcpkg will automatically handle these dependencies.

3. For the gmsh library, the vcpkg-wrapped version is not used here. Instead, the system package manager on Linux is used to import gmsh (you can also download the official gmsh SDK). This is mainly because the gmsh version imported by vcpkg has too few compilation options enabled, and many features cannot be used. The correct way is to manually compile gmsh in the project, but the dependencies of gmsh are numerous and complex, so it is not currently planned to do so.

4. The testing framework used is google-test, and some tests require integration with mpi.

5. Doxygen is used to generate code documentation (which uses the dot component of [Graphviz](https://www.graphviz.org) to generate relationship diagrams). The plugin cschlosser.doxdocgen is used to generate comments for each function, as well as file headers.

### develop specification

1. The naming conventions for the code are based on the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), and files are formatted using clang-format. Variable naming largely follows the Google C++ Style Guide, and clang-tidy is used for some checks. Static code analysis is also implemented with clang-tidy. The formatting style for files follows Google's style guide, and the maximum line width has been adjusted to 120, with clang-format used for code formatting.

2. clangd is used as the intelliSenseEngine, so the intelliSenseEngine in the ms-vscode.cpptools plugin needs to be disabled.
```json
"C_Cpp.intelliSenseEngine": "Disabled"
```
Compared to the intelliSenseEngine in `ms-vscode.cpptools` , clangd supports code completion and error prompts across files. The configuration for clangd is written in settings.json.
```json
"clangd.arguments": [
    "--all-scopes-completion",
    "--background-index",
    "--clang-tidy",
    "--completion-style=detailed",
    "--enable-config",
    "--function-arg-placeholders=false",
    "--header-insertion=never",
    "--j=4",
    "--pch-storage=memory"
]
```

3. Header file checking uses include-what-you-use, which is integrated in cmake. During compilation, excess header files are checked. Starting from 2023-03-19, there is an rpm package available for [iwyu](https://src.fedoraproject.org/rpms/iwyu) , so manual compilation is no longer required.

4. Here, the [eigendbg](https://github.com/dmillard/eigengdb) library was used to optimize the display of matrices while debugging eigen using gdb.

5. The development tools mentioned above are integrated with cmake, and can be run through vscode's cmake plugin by changing the target.

### C/C++ detail


## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FSubrosaDG-dev%2FSubrosaDG.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FSubrosaDG-dev%2FSubrosaDG?ref=badge_large)