'''
' @file draw_figures.py
' @brief Draw figures for the notes of DG method.
'
' @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
' @date 2023-07-31
'
' @version 0.1.0
' @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
' SubrosaDG is free software and is distributed under the MIT license.
'''

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from matplotlib import cm
from scipy import interpolate

plt.rc('text', usetex=True)
plt.rc('text.latex', preamble=r'\usepackage{amsmath}\usepackage{bm}')

fig = plt.figure(figsize=(12, 4))
fig.set_tight_layout(True)

ax = fig.add_subplot(1, 3, 1)

x = np.array([0, 1, 0, 0])
y = np.array([0, 0, 1, 0])
n = np.array([1, 2, 3])
ax.plot(x, y, marker='o')
for i in range(3):
    ax.annotate(f'${n[i]}$', (x[i], y[i]), xytext=(x[i] + 0.01, y[i] + 0.01))
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.spines.right.set_visible(False)
ax.spines.top.set_visible(False)

ax = fig.add_subplot(1, 3, 2)

x = np.array([0, 0.5, 1, 0.5, 0, 0, 0])
y = np.array([0, 0, 0, 0.5, 1, 0.5, 0])
n = np.array([1, 4, 2, 5, 3, 6,])
ax.plot(x, y, marker='o')
for i in range(6):
    ax.annotate(f'${n[i]}$', (x[i], y[i]), xytext=(x[i] + 0.01, y[i] + 0.01))
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.spines.right.set_visible(False)
ax.spines.top.set_visible(False)

ax = fig.add_subplot(1, 3, 3)

x = np.array([0, 0.33, 0.66, 1, 0.66, 0.33, 0, 0, 0, 0])
y = np.array([0, 0, 0, 0, 0.33, 0.66, 1, 0.66, 0.33, 0])
n = np.array([1, 4, 5, 2, 6, 7, 3, 8, 9])
ax.scatter(0.33, 0.33, marker='o')
ax.annotate('$10$', (0.33, 0.33), xytext=(0.34, 0.34))
ax.plot(x, y, marker='o')
for i in range(9):
    ax.annotate(f'${n[i]}$', (x[i], y[i]), xytext=(x[i] + 0.01, y[i] + 0.01))
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.spines.right.set_visible(False)
ax.spines.top.set_visible(False)

plt.savefig('figures/tri-reference.pdf')

plt.clf()

fig = plt.figure(figsize=(12, 4))
fig.set_tight_layout(True)

ax = fig.add_subplot(1, 3, 1)

x = np.array([-1, 1, 1, -1, -1])
y = np.array([-1, -1, 1, 1, -1])
n = np.array([1, 2, 3, 4])
ax.plot(x, y, marker='o')
for i in range(4):
    ax.annotate(f'${n[i]}$', (x[i], y[i]), xytext=(x[i] + 0.02, y[i] + 0.02))
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.yaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax.spines.right.set_visible(False)
ax.spines.top.set_visible(False)

ax = fig.add_subplot(1, 3, 2)

x = np.array([-1, 0, 1, 1, 1, 0, -1, -1, -1])
y = np.array([-1, -1, -1, 0, 1, 1, 1, 0, -1])
n = np.array([1, 5, 2, 6, 3, 7, 4, 8])
ax.scatter(0, 0, marker='o')
ax.annotate('$9$', (0, 0), xytext=(0.02, 0.02))
ax.plot(x, y, marker='o')
for i in range(8):
    ax.annotate(f'${n[i]}$', (x[i], y[i]), xytext=(x[i] + 0.02, y[i] + 0.02))
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.yaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax.spines.right.set_visible(False)
ax.spines.top.set_visible(False)

ax = fig.add_subplot(1, 3, 3)

x = np.array([-1, -0.33, 0.33, 1, 1, 1, 1, 0.33, -0.33, -1, -1, -1, -1])
y = np.array([-1, -1, -1, -1, -0.33, 0.33, 1, 1, 1, 1, 0.33, -0.33, -1])
n = np.array([1, 5, 6, 2, 7, 8, 3, 9, 10, 4, 11, 12])
ax.scatter([-0.33, 0.33, -0.33, 0.33], [-0.33, -0.33, 0.33, 0.33], marker='o')
ax.annotate('$13$', (-0.33, -0.33), xytext=(-0.31, -0.31))
ax.annotate('$14$', (0.33, -0.33), xytext=(0.35, -0.31))
ax.annotate('$15$', (0.33, 0.33), xytext=(0.35, 0.35))
ax.annotate('$16$', (-0.33, 0.33), xytext=(-0.31, 0.35))
ax.plot(x, y, marker='o')
for i in range(12):
    ax.annotate(f'${n[i]}$', (x[i], y[i]), xytext=(x[i] + 0.02, y[i] + 0.02))
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.yaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax.spines.right.set_visible(False)
ax.spines.top.set_visible(False)

plt.savefig('figures/quad-reference.pdf')

plt.clf()

fig = plt.figure(figsize=(10, 6))
fig.set_tight_layout(True)

N = 200

x = np.linspace(0, 1, N, endpoint=True)
y = np.linspace(0, 1, N, endpoint=True)
xx, yy = np.meshgrid(x, y)

x = np.array([0, 1, 0, 0.5, 0.5, 0])
y = np.array([0, 0, 1, 0, 0.5, 0.5])

tri_phi_3 = np.array([0, 0, 1, 0, 0, 0])
tri_phi_3_3d = -yy + 2 * yy**2
tri_phi_3_3d[xx + yy > 1] = np.nan

tri_phi_4 = np.array([0, 0, 0, 1, 0, 0])
tri_phi_4_3d = 4 * xx - 4 * xx**2 - 4 * xx * yy
tri_phi_4_3d[xx + yy > 1] = np.nan

ax = fig.add_subplot(1, 2, 1, projection='3d')

ax.scatter(x, y, tri_phi_3, c='r')
ax.plot_surface(xx, yy, tri_phi_3_3d, cmap=cm.viridis)
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.set_title(r'$\phi_{3}^{2}(\xi,\eta)=-\eta+2\eta^{2}$')

ax = fig.add_subplot(1, 2, 2, projection='3d')

ax.scatter(x, y, tri_phi_4, c='r')
ax.plot_surface(xx, yy, tri_phi_4_3d, cmap=cm.viridis)
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.set_title(r'$\phi_{4}^{2}(\xi,\eta)=4\xi-4\xi^{2}-4\xi\eta$')

plt.savefig('figures/tri-basis-fun.pdf')

plt.clf()

fig = plt.figure(figsize=(10, 6))
fig.set_tight_layout(True)

x = np.linspace(-1, 1, N, endpoint=True)
y = np.linspace(-1, 1, N, endpoint=True)
xx, yy = np.meshgrid(x, y)

x = np.array([-1, 1, 1, -1, 0, 1, 0, -1, 0])
y = np.array([-1, -1, 1, 1, -1, 0, 1, 0, 0])

quad_phi_4 = np.array([0, 0, 0, 1, 0, 0, 0, 0, 0])
quad_phi_4_3d = (-xx * yy + xx**2 * yy - xx * yy**2 + xx**2 * yy**2) / 4

quad_phi_5 = np.array([0, 0, 0, 0, 1, 0, 0, 0, 0])
quad_phi_5_3d = (-yy + yy**2 + xx**2 * yy - xx**2 * yy**2) / 2

ax = fig.add_subplot(1, 2, 1, projection='3d')

ax.scatter(x, y, quad_phi_4, c='r')
ax.plot_surface(xx, yy, quad_phi_4_3d, cmap=cm.viridis)
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.xaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax.yaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax.set_title(r'$\phi_{4}^{2}(\xi,\eta)=\dfrac{1}{4}(-\xi\eta+\xi^{2}\eta-\xi\eta^{2}+\xi^{2}\eta^{2})$')

ax = fig.add_subplot(1, 2, 2, projection='3d')

ax.scatter(x, y, quad_phi_5, c='r')
ax.plot_surface(xx, yy, quad_phi_5_3d, cmap=cm.viridis)
ax.set_xlabel(r'$\xi$')
ax.set_ylabel(r'$\eta$')
ax.xaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax.yaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax.set_title(r'$\phi_{5}^{2}(\xi,\eta)=\dfrac{1}{2}(-\eta+\eta^{2}+\xi^{2}\eta-\xi^{2}\eta^{2})$')

plt.savefig('figures/quad-basis-fun.pdf')

plt.clf()

fig = plt.figure(figsize=(10, 4))
fig.set_tight_layout(True)

ax1 = fig.add_subplot(1, 2, 1)

x = np.array([-1, -0.33, 0.33, 1, 1, 1, 1, 0.33, -0.33, -1, -1, -1, -1])
y = np.array([-1, -1, -1, -1, -0.33, 0.33, 1, 1, 1, 1, 0.33, -0.33, -1])
ax1.scatter([-0.33, 0.33, -0.33, 0.33], [-0.33, -0.33, 0.33, 0.33], marker='o')
ax1.plot(x, y, marker='o')
ax1.set_xlabel(r'$\xi$')
ax1.set_ylabel(r'$\eta$')
ax1.yaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax1.spines.right.set_visible(False)
ax1.spines.top.set_visible(False)

ax2 = fig.add_subplot(1, 2, 2)

x = np.array([2.0, 3.0, 4.0, 5.0])
y = np.array([2.0, 1.9, 2.1, 2.0])
x_new = np.linspace(2.0, 5.0, 100)
bspline = interpolate.make_interp_spline(x, y)
y_new = bspline(x_new)
ax2.plot(x_new, y_new, color='C0')
ax2.scatter(x, y, marker='o', color='C0')
x = np.array([5.0, 4.9, 5.1, 5.0])
y = np.array([2.0, 3.0, 4.0, 5.0])
y_new = np.linspace(2.0, 5.0, 100)
bspline = interpolate.make_interp_spline(y, x)
x_new = bspline(y_new)
ax2.plot(x_new, y_new, color='C0')
ax2.scatter(x, y, marker='o', color='C0')
x = np.array([2.0, 3.0, 4.0, 5.0])
y = np.array([5.0, 4.9, 5.1, 5.0])
x_new = np.linspace(2.0, 5.0, 100)
bspline = interpolate.make_interp_spline(x, y)
y_new = bspline(x_new)
ax2.plot(x_new, y_new, color='C0')
ax2.scatter(x, y, marker='o', color='C0')
x = np.array([2.0, 1.9, 2.1, 2.0])
y = np.array([2.0, 3.0, 4.0, 5.0])
y_new = np.linspace(2.0, 5.0, 100)
bspline = interpolate.make_interp_spline(y, x)
x_new = bspline(y_new)
ax2.plot(x_new, y_new, color='C0')
ax2.scatter(x, y, marker='o', color='C0')
ax2.scatter([2.9, 3.9, 3.1, 4.1], [2.9, 3.1, 3.9, 4.1], marker='o')
ax2.set_xlabel(r'$x$')
ax2.set_ylabel(r'$y$')
ax2.xaxis.set_major_locator(ticker.MultipleLocator(0.5))
ax2.spines.right.set_visible(False)
ax2.spines.top.set_visible(False)

arrowprops = dict(arrowstyle="->", connectionstyle="arc3, rad=0.2", color='blue')

arrow1_2 = plt.annotate('', xy=(-0.18, 0.49), xycoords=ax2.transAxes, xytext=(1.05, 0.48), textcoords=ax1.transAxes, arrowprops=arrowprops)
plt.text(0.445, 0.6, r'$\displaystyle\mathbf{x}=\sum_{i=1}^{N_{k}}\phi_{i}^{k}(\bm{\xi})\mathbf{x}_{i}$', transform=fig.transFigure)

ax1.set_aspect('equal', adjustable='box')
ax2.set_aspect('equal', adjustable='box')

plt.savefig('figures/quad-isoparametric.pdf')
