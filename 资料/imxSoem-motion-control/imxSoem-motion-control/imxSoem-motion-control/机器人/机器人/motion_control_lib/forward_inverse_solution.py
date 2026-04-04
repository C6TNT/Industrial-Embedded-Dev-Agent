#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sympy
from sympy import Matrix
import torch
import sympy
from sympy import Matrix, eye

# 全局变量存储总变换矩阵（多个矩阵右乘结果）
global_total_matrix = None
# 存储所有theta符号（用于后续替换）
global_thetas = []

def single_matrix_generate(a_val, d_val, alpha_val, theta_sym):
    """生成单个关节的矩阵（接收指定的theta符号，支持多变量）"""
    matrix_expr = sympy.Matrix([
        [sympy.cos(theta_sym), -sympy.sin(theta_sym), 0, a_val],
        [sympy.sin(theta_sym)*sympy.cos(alpha_val), sympy.cos(theta_sym)*sympy.cos(alpha_val), -sympy.sin(alpha_val), -d_val*sympy.sin(alpha_val)],
        [sympy.sin(theta_sym)*sympy.sin(alpha_val), sympy.cos(theta_sym)*sympy.sin(alpha_val), sympy.cos(alpha_val), d_val*sympy.cos(alpha_val)],
        [0, 0, 0, 1]
    ])
    return matrix_expr

def all_matrix_generate(joint_params):
    """
    生成多关节总变换矩阵（右乘）
    joint_params: 关节参数列表，每个元素为(a, d, alpha)，长度对应theta数量
    例如：[(a1, d1, alpha1), (a2, d2, alpha2)] 对应2个theta
    """
    global global_total_matrix, global_thetas
    # 重置全局变量
    global_thetas = []
    # 初始总矩阵为单位矩阵
    global_total_matrix = eye(4)

    # 为每个关节生成矩阵并右乘
    for i, (a, d, alpha) in enumerate(joint_params):
        # 定义唯一的theta符号（theta1, theta2, ...）
        theta_sym = sympy.Symbol(f'theta{i+1}')
        global_thetas.append(theta_sym)

        # 生成当前关节的矩阵
        joint_matrix = single_matrix_generate(a, d, alpha, theta_sym)

        # 右乘到总矩阵（总矩阵 = 总矩阵 * 当前关节矩阵）
        global_total_matrix = global_total_matrix * joint_matrix

    for i in range(4):
        row = [str(global_total_matrix[i, j]) for j in range(4)]
        print(f"[{', '.join(row)}]")
    return global_total_matrix

def get_end_position(*thetas):
    """
    接收多个theta值，返回总矩阵的16个元素
    *thetas: 与关节数量对应的theta值，例如theta1, theta2
    """
    if global_total_matrix is None:
        raise ValueError("请先调用all_matrix_generate生成总矩阵")
    if len(thetas) != len(global_thetas):
        raise ValueError(f"需传入{len(global_thetas)}个theta值，实际传入{len(thetas)}个")

    # 替换所有theta符号
    matrix_sub = global_total_matrix
    for theta_sym, theta_val in zip(global_thetas, thetas):
        matrix_sub = matrix_sub.subs(theta_sym, theta_val)

    # 提取16个元素（行优先）
    return [float(matrix_sub[i, j]) for i in range(4) for j in range(4)]
