## SubrosaDG Project Specification

### git collaborate

1. 本项目拉取方式为 `git clone ssh://git@101.34.44.55:50050/~/SubrosaDG.git` 。

2. 服务器端项目分支为master，建议拉取后自己创建本地dev分支（这个分支不需要上传），主要提交在本地dev分支上进行，dev分支上的commit可以随意提交，在部分模块完成后再合并到本地master分支即可。

3. 拉取远端代码可以直接切换到master分支并 `git pull` 即可。

4. 建议在将本地dev分支合并到本地master分支之前先拉取远端的master分支进行更新，以免合并后将本地master分支推送到远端master分支时产生冲突。

5. 在使用master分支合并dev分支建议 `git merge --squash dev` ，这里加上`--squash` 的选项是为了把本地dev分支前产生的一系列commit统一成一个commit再合并到master分支。

6. 在使用master分支合并dev分支并推送到远端master分支后dev分支会落后master分支一次提交
```bash
... o ---- A ---- B ---- C  origin/master (upstream work)
            \
             C ---- D  dev (your work)
```
此时如果在将dev分支合并到master分支前master分支都没有人修改过，那么master分支和dev分支上的内容是相同的，此时不需要再操作。
但是如果有人在合并前修改了master分支上的内容，那么两个分支上的内容就会有差异，就需要在将远端master分支更新后同时更新本地的dev分支，由于上次合并处理成了一次提交，这里可以使用 `cherry-pick` ，也就是重复上次提交的内容
```bash
git cherry-pick ${branch master merge commit id}
```
如果之前进行过冲突处理这里需要再进行一遍，以master分支中的为准，这之后git的提交路线应该为
```bash
... o ---- A ---- B ---- C  origin/master (upstream work)
            \
             C ---- D ---- M dev (your work)
```
这样操作是为了保护dev分之以前的提交，使用其他方法均会对dev分支的提交历史造成影响。

7. 建议在vscode的setting.json中加上下面这个设置以避免上传dev分支，第三个是防止vscode搜索上层git库。
```json
"git.showActionButton": {
    "publish": false,
    "sync": false
}
"git.showPushSuccessNotification": true,
"git.autoRepositoryDetection": false,
```

### build toolchain

1. 使用cmake生成Makefile并用Ninja进行编译，编译器为clang，并使用ccache进行编译加速。

2. 使用vcpkg引入第三方库，这里使用到的库有libconfig、fmt、spdlog、eigen3、hdf5、cgns、openmp（编译器自带）和openmpi（暂未链接）。虽然项目在cmake集成了vcpkg，但建议还是先在本地安装过一遍，项目构建的时候会直接将编译缓存复制进来。这里需要`vcpkg install libconfig fmt spdlog eigen3 cgns`，某些组件之间会相互依赖，例如cgns依赖hdf5，vcpkg会自动处理这些依赖。对于mpi，目前还没有链接。

3. 整体风格这里参考[Google开源风格指南](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/contents/)，文件使用clang-format进行格式化，变量命名大体上参考了Google开源风格指南，部分也使用了clang-tidy进行检查，代码的静态检查也是clang-tidy实现的。

4. 代码文档使用Doxygen进行生成，这里使用插件cschlosser.doxdocgen来生成每个函数的注释，同时生成文件头。

5. 测试框架选用的是google-test，部分的测试需要集成mpi。

6. 这里intelliSenseEngine使用的是clangd，因此需要屏蔽ms-vscode.cpptools插件本身的intelliSenseEngine。
```json
"C_Cpp.intelliSenseEngine": "Disabled"
```

7. 以上所有的工具均使用cmake进行了集成，基本都可以在cmake中通过更改target的方式直接运行。

### C/C++ detail
