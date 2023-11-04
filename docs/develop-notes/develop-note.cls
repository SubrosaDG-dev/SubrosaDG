%%
% @file develop-note.cls
% @brief The latex stencil of develop note.
%
% @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
% @date 2023-07-18
%
% @version 0.1.0
% @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
% SubrosaDG is free software and is distributed under the MIT license.
%%

\def\@pslReleaseDate{2023/07/18}
\def\@pslVersion{0.1.0}
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{develop-note}[\@pslReleaseDate, \@pslVersion]

\LoadClass[10pt,a4paper]{article}

\RequirePackage{kvoptions}
\SetupKeyvalOptions{
  family=stencil,
  prefix=stencil@,
  setkeys=\kvsetkeys
}
\def\stencilset{\kvsetkeys{stencil@value}}
\def\stencil@define #1{
  \define@key{stencil}{#1}{\csname #1\endcsname{##1}}
  \expandafter\gdef\csname #1\endcsname##1{
    \expandafter\gdef\csname stencil@#1\endcsname{##1}}
  \csname #1\endcsname{}
}
\stencil@define{value@name}

\RequirePackage[a4paper,hmargin=1.5cm,vmargin=2.5cm,headheight=16pt]{geometry}

\RequirePackage[short,24hr]{datetime}

\RequirePackage{microtype}

\RequirePackage{setspace}

\setlength{\lineskiplimit}{3pt}
\setlength{\lineskip}{3pt}

\linespread{1.5}

\setlength\parindent{0em}

\title{\textbf{\stencil@value@name}}
\author{}
\date{}

\RequirePackage{array}

\RequirePackage{booktabs}

\RequirePackage{enumerate}

\RequirePackage{graphicx}

\RequirePackage{float}

\RequirePackage{tikz}
\RequirePackage{pgfplots}

\RequirePackage{algorithm2e}

\RequirePackage{xcolor}

\RequirePackage{siunitx}

\RequirePackage[fleqn]{amsmath}
\RequirePackage{amssymb}
\RequirePackage{amsthm}

\RequirePackage{nicematrix}
\NiceMatrixOptions{cell-space-limits=2pt}

\RequirePackage{esint}

% \RequirePackage{annotate-equations}

\RequirePackage[version=4]{mhchem}

\everymath{\displaystyle}

\RequirePackage{mathrsfs}

\RequirePackage{bm}

\RequirePackage{hyperref}
\hypersetup{
  colorlinks,linkcolor=black,anchorcolor=black,citecolor=black,urlcolor=teal,
  bookmarksnumbered=true
}

\AtBeginDocument{\maketitle}
