## SubrosaDG Project Specification

### git collaborate

1. 本项目拉取方式为 `git clone git@github.com:callm1101/SubrosaDG.git` .

2. 服务器端项目分支为 `master` ,建议拉取后自己创建本地 `dev` 分支（这个分支不需要上传）,主要提交在本地 `dev` 分支上进行, `dev` 分支上的 `commit` 可以随意提交,在部分模块完成后再合并到本地 `master` 分支即可.

3. 拉取远端代码可以直接切换到 `master` 分支并 `git pull` 即可.

4. 建议在将本地 `dev` 分支合并到本地 `master` 分支之前先拉取远端的 `master` 分支进行更新,以免合并后将本地 `master` 分支推送到远端 `master` 分支时产生冲突.

5. 在使用 `master` 分支合并 `dev` 分支建议 `git merge --squash dev` ,这里加上`--squash` 的选项是为了把本地 `dev` 分支前产生的一系列 `commit` 统一成一个 `commit` 再合并到 `master` 分支.

6. 在使用 `master` 分支合并 `dev` 分支并推送到远端 `master` 分支后 `dev` 分支会落后 `master` 分支一次提交
```bash
... o ---- A ---- B ---- C  origin/master (upstream work)
            \
             C ---- D  dev (your work)
```
此时如果在将 `dev` 分支合并到 `master` 分支前 `master` 分支都没有人修改过,那么 `master` 分支和 `dev` 分支上的内容是相同的,此时不需要再操作.
但是如果有人在合并前修改了 `master` 分支上的内容,那么两个分支上的内容就会有差异,就需要在将远端 `master` 分支更新后同时更新本地的 `dev` 分支,由于上次合并处理成了一次提交,这里可以使用 `cherry-pick` ,也就是重复上次提交的内容
```bash
git cherry-pick ${branch master merge commit id}
```
如果之前进行过冲突处理这里需要再进行一遍,这时候你可以使用 `git checkout master ${filename}` ,以 `master` 分支中的代码为准,这之后 `git` 的提交路线应该为
```bash
... o ---- A ---- B ---- C  origin/master (upstream work)
            \
             C ---- D ---- M dev (your work)
```
这样操作是为了保护 `dev` 分之以前的提交,使用其他方法均会对 `dev` 分支的提交历史造成影响.

7. 建议在 `vscode` 的 `setting.json` 中加上下面这个设置以避免上传 `dev` 分支,第三个设置可以防止 `vscode` 搜索上层 `git` 库.
```json
"git.showActionButton": {
    "publish": false,
    "sync": false
}
"git.showPushSuccessNotification": true,
"git.autoRepositoryDetection": false,
```

### build toolchain

1. 使用 `cmake` 生成 `build.ninja` 并用 `ninja` 进行编译,编译器为 `clang` ,并使用 `ccache` 进行编译加速.

2. 项目部分使用 `vcpkg` 引入第三方库,这里使用到的库有 `libconfig` , `fmt` ,`spdlog` , `eigen3` , `openmp` （编译器自带）, `openmpi` 和 `dbg-macro` .虽然项目在 `cmake` 中集成了 `vcpkg` ,但建议还是先在本地安装过一遍,项目构建的时候会直接将编译缓存复制进来,某些组件之间会相互依赖, `vcpkg` 会自动处理这些依赖.

3. 对于 `gmsh` 库的引入,这里并没有使用 `vcpkg` 中封装好的 `gmsh` ,而是使用 linux 系统中的包管理器来引入 `gmsh` (这里也可以去下载 `gmsh` 官方的 sdk),主要是由于 `vcpkg` 引入的 `gmsh` 开启的编译选项过少,很多功能并不能使用,这里其实正确的方式是在项目中手动编译 `gmsh` ,但是 `gmsh` 本身的依赖多且复杂,目前暂时不打算这样操作.

4. 测试框架选用的是 `google-test` ,部分的测试需要集成 `mpi` .

5. 代码文档使用 `Doxygen` 进行生成(使用 `Doxygen` 生成文档时同时使用到了 [`Graphviz`](https://www.graphviz.org) 中的 `dot` 组件来生成关系图),这里使用插件 `cschlosser.doxdocgen` 来生成每个函数的注释,同时生成文件头.

### develop specification

1. 代码的命名规范这里参考 [Google 开源风格指南](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/contents/),文件使用 `clang-format` 进行格式化,变量命名大体上参考了 Google 开源风格指南,部分也使用了 `clang-tidy` 进行检查,代码的静态检查也是 `clang-tidy` 实现的.同样文件的 `format` 格式也是参考的 Google 的格式,并且将最大行宽调整到了 120 ,这里用 `clang-format` 进行代码的格式化.

2. 这里 `intelliSenseEngine` 使用的是 `clangd` ,因此需要屏蔽 `ms-vscode.cpptools` 插件本身的 `intelliSenseEngine` .
```json
"C_Cpp.intelliSenseEngine": "Disabled"
```
`clangd` 相较于 `ms-vscode.cpptools` 的 `intelliSenseEngine` 来说可以支持跨文件的代码补全以及错误提示,这里 `clangd` 的配置写在 `settings.json` 中
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

3. 头文件检查采用了 `include-what-you-use` ,这部分集成在了 `cmake` 中,编译时会检查多余的包含头文件,从 2023-03-19 开始 [`iwyu`](https://src.fedoraproject.org/rpms/iwyu) 有了 rpm 包,不用在手动编译了.

4. 上述部分开发工具使用 `cmake` 进行了集成,可以在 `vscode` 的 `cmake` 插件中通过更改 `target` 的方式运行.

### C/C++ detail
