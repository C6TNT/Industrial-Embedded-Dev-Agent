//#include "python3.7m/Python.h"
//#include <iostream>
//#include <vector>
//static PyObject* pModule = NULL;
//// 假设你的Python文件名为add_module.py
//void python_init(){
//    // 初始化Python解释器
//    Py_Initialize();
//    // 检查Python是否初始化成功
//    if (!Py_IsInitialized()) {
//        std::cerr << "Python初始化失败!" << std::endl;
//        return;
//    }
//    // 添加Python文件所在的路径到sys.path
//    // 请将路径替换为你的Python文件实际所在路径
//    PyRun_SimpleString("import sys");
//    PyRun_SimpleString("sys.path.append('/home/hh')");
//    // 导入Python模块（不要加.py扩展名）
//    pModule = PyImport_ImportModule("forward_inverse_solution");
//    if (!pModule) {
//        PyErr_Print();
//        throw std::runtime_error("无法导入Python模块");
//    }
//}
//void python_set_matrix(int axisNum, double joint_params[][3]) {
//    // 1. 参数合法性检查
//    if (axisNum <= 0 || !joint_params) {
//        throw std::invalid_argument("关节数量必须为正数，且参数数组不能为空");
//    }

//    // 2. 获取Python函数
//    PyObject* pFunc = PyObject_GetAttrString(pModule, "all_matrix_generate");
//    if (!pFunc || !PyCallable_Check(pFunc)) {
//        PyErr_Print();
//        Py_DECREF(pModule);
//        throw std::runtime_error("无法获取all_matrix_generate函数");
//    }

//    // 3. 创建外层列表（存储所有关节参数）
//    PyObject* pJointList = PyList_New(axisNum);
//    if (!pJointList) {
//        PyErr_Print();
//        Py_DECREF(pFunc);
//        throw std::runtime_error("无法创建关节参数外层列表");
//    }

//    // 4. 循环填充每个关节的参数（从二维数组提取）
//    for (int i = 0; i < axisNum; ++i) {
//        // 创建子列表（存储单个关节的3个参数）
//        PyObject* pSubList = PyList_New(3);
//        if (!pSubList) {
//            PyErr_Print();
//            Py_DECREF(pJointList);
//            Py_DECREF(pFunc);
//            throw std::runtime_error("无法创建关节参数子列表");
//        }

//        // 从二维数组获取当前关节的a、d、alpha（固定索引0、1、2）
//        double a = joint_params[i][0];       // 第i个关节的a值
//        double d = joint_params[i][1];       // 第i个关节的d值
//        double alpha = joint_params[i][2];   // 第i个关节的alpha值

//        // 设置子列表的三个元素
//        PyList_SetItem(pSubList, 0, PyFloat_FromDouble(a));
//        PyList_SetItem(pSubList, 1, PyFloat_FromDouble(d));
//        PyList_SetItem(pSubList, 2, PyFloat_FromDouble(alpha));

//        // 将子列表添加到外层列表
//        PyList_SetItem(pJointList, i, pSubList);
//    }

//    // 5. 准备函数参数（仅一个参数：关节参数列表）
//    PyObject* pArgs = PyTuple_New(1);
//    PyTuple_SetItem(pArgs, 0, pJointList);

//    // 6. 调用Python函数
//    PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
//    if (!pResult) {
//        PyErr_Print();
//        Py_DECREF(pArgs);
//        Py_DECREF(pFunc);
//        throw std::runtime_error("调用all_matrix_generate失败");
//    }

//    // 7. 释放资源（保留pModule供后续使用）
//    Py_DECREF(pResult);
//    Py_DECREF(pArgs);
//    Py_DECREF(pFunc);
//}

//void python_get_matrix(int axisNum,double *theta,double *matrix) {
//    try {
//        // 获取add函数
//        PyObject* pFunc = PyObject_GetAttrString(pModule, "get_end_position");
//        if (!pFunc || !PyCallable_Check(pFunc)) {
//            PyErr_Print();
//            Py_DECREF(pModule);
//            throw std::runtime_error("无法获取add函数或函数不可调用");
//        }

//        // 准备函数参数，这里以两个整数为例
//        PyObject* pArgs = PyTuple_New(axisNum);  // 创建有n个元素的元组
//        for(int i=0;i<axisNum;i++){
//            PyTuple_SetItem(pArgs, i, PyFloat_FromDouble(theta[i]));  // 设置第一个参数
//        }

//        // 调用函数
//        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
//        if (!pResult) {
//            PyErr_Print();
//            Py_DECREF(pArgs);
//            Py_DECREF(pFunc);
//            Py_DECREF(pModule);
//            throw std::runtime_error("调用add函数失败");
//        }

//        int len = PyList_Size(pResult);
//        // 解析函数返回值

//        // 逐个提取元素
//        for (int i = 0; i < len; i++) {
//            PyObject* item = PyList_GetItem(pResult, i);  // 取第i个元素
//            matrix[i] = PyFloat_AsDouble(item);         // 转为double
//        }
//        // 释放资源
//        Py_DECREF(pResult);
//        Py_DECREF(pArgs);
//        Py_DECREF(pFunc);
//    }

//    catch (const std::exception& e) {
//        std::cerr << "错误: " << e.what() << std::endl;
//    }

//}

//void python_kinematics(int axisNum, double* target_matrix, double* current_theta, double* solved_theta) {
//    double lr = 1e-3;
//    int epochs = 1000;
//    try {
//        // 1. 获取inverse_kinematics函数对象
//        PyObject* pFunc = PyObject_GetAttrString(pModule, "inverse_kinematics");
//        if (!pFunc || !PyCallable_Check(pFunc)) {
//            PyErr_Print();
//            Py_DECREF(pModule);
//            throw std::runtime_error("无法获取inverse_kinematics函数或函数不可调用");
//        }

//        // 2. 构造参数：current_thetas（列表）
//        PyObject* pCurrentThetas = PyList_New(axisNum);
//        for (int i = 0; i < axisNum; i++) {
//            PyList_SetItem(pCurrentThetas, i, PyFloat_FromDouble(current_theta[i]));
//        }

//        // 3. 构造参数：target_pose_16（16维列表）
//        PyObject* pTargetPose = PyList_New(16);
//        for (int i = 0; i < 16; i++) {
//            PyList_SetItem(pTargetPose, i, PyFloat_FromDouble(target_matrix[i]));
//        }

//        // 4. 构造参数元组（包含4个参数：current_thetas, target_pose_16, lr, epochs）
//        PyObject* pArgs = PyTuple_New(4);
//        PyTuple_SetItem(pArgs, 0, pCurrentThetas);       // 第0个参数：当前关节角列表
//        PyTuple_SetItem(pArgs, 1, pTargetPose);          // 第1个参数：目标位姿16维列表
//        PyTuple_SetItem(pArgs, 2, PyFloat_FromDouble(lr)); // 第2个参数：学习率
//        PyTuple_SetItem(pArgs, 3, PyLong_FromLong(epochs)); // 第3个参数：迭代次数

//        // 5. 调用函数
//        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
//        if (!pResult) {
//            PyErr_Print();
//            Py_DECREF(pArgs);
//            Py_DECREF(pFunc);
//            Py_DECREF(pModule);
//            throw std::runtime_error("kin11");
//        }

//        // 6. 解析返回值（关节角列表，长度为axisNum）
//        if (PyList_Size(pResult) != axisNum) {
//            Py_DECREF(pResult);
//            Py_DECREF(pArgs);
//            Py_DECREF(pFunc);
//            throw std::runtime_error("kin22");
//        }
//        for (int i = 0; i < axisNum; i++) {
//            PyObject* item = PyList_GetItem(pResult, i);
//            solved_theta[i] = PyFloat_AsDouble(item);
//        }

//        // 7. 释放资源
//        Py_DECREF(pResult);
//        Py_DECREF(pArgs);
//        Py_DECREF(pFunc);

//    } catch (const std::exception& e) {
//        std::cerr << "错误: " << e.what() << std::endl;
//    }
//}
