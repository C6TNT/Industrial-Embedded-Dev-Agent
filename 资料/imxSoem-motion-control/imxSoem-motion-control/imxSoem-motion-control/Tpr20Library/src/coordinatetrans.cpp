#include <iostream>
#include <math.h>
#include "coordinatetrans.h"

#define PI 3.141592653589793238

void matrix_inverse(double(*a)[3], double(*b)[3])
{
    int i, j, k;
    double max, temp;
    // 定义一个临时矩阵t
    double t[3][3];
    // 将a矩阵临时存放在矩阵t[n][n]中
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            t[i][j] = a[i][j];
        }
    }
    // 初始化B矩阵为单位矩阵
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            b[i][j] = (i == j) ? (double)1 : 0;
        }
    }
    // 进行列主消元，找到每一列的主元
    for (i = 0; i < 3; i++)
    {
        max = t[i][i];
        // 用于记录每一列中的第几个元素为主元
        k = i;
        // 寻找每一列中的主元元素
        for (j = i + 1; j < 3; j++)
        {
            if (fabs(t[j][i]) > fabs(max))
            {
                max = t[j][i];
                k = j;
            }
        }
        //cout<<"the max number is "<<max<<endl;
        // 如果主元所在的行不是第i行，则进行行交换
        if (k != i)
        {
            // 进行行交换
            for (j = 0; j < 3; j++)
            {
                temp = t[i][j];
                t[i][j] = t[k][j];
                t[k][j] = temp;
                // 伴随矩阵B也要进行行交换
                temp = b[i][j];
                b[i][j] = b[k][j];
                b[k][j] = temp;
            }
        }
        if (t[i][i] == 0)
        {
            //cout << "\nthe matrix does not exist inverse matrix\n";
            break;
        }
        // 获取列主元素
        temp = t[i][i];
        // 将主元所在的行进行单位化处理
        //cout<<"\nthe temp is "<<temp<<endl;
        for (j = 0; j < 3; j++)
        {
            t[i][j] = t[i][j] / temp;
            b[i][j] = b[i][j] / temp;
        }
        for (j = 0; j < 3; j++)
        {
            if (j != i)
            {
                temp = t[j][i];
                //消去该列的其他元素
                for (k = 0; k < 3; k++)
                {
                    t[j][k] = t[j][k] - temp * t[i][k];
                    b[j][k] = b[j][k] - temp * b[i][k];
                }
            }

        }

    }
}

void matrix_inverse4(double(*a)[4], double(*b)[4])
{
    int i, j, k;
    double max, temp;
    // 定义一个临时矩阵t
    double t[4][4];
    // 将a矩阵临时存放在矩阵t[n][n]中
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            t[i][j] = a[i][j];
        }
    }
    // 初始化B矩阵为单位矩阵
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            b[i][j] = (i == j) ? (double)1 : 0;
        }
    }
    // 进行列主消元，找到每一列的主元
    for (i = 0; i < 4; i++)
    {
        max = t[i][i];
        // 用于记录每一列中的第几个元素为主元
        k = i;
        // 寻找每一列中的主元元素
        for (j = i + 1; j < 4; j++)
        {
            if (fabs(t[j][i]) > fabs(max))
            {
                max = t[j][i];
                k = j;
            }
        }
        //cout<<"the max number is "<<max<<endl;
        // 如果主元所在的行不是第i行，则进行行交换
        if (k != i)
        {
            // 进行行交换
            for (j = 0; j < 4; j++)
            {
                temp = t[i][j];
                t[i][j] = t[k][j];
                t[k][j] = temp;
                // 伴随矩阵B也要进行行交换
                temp = b[i][j];
                b[i][j] = b[k][j];
                b[k][j] = temp;
            }
        }
        if (t[i][i] == 0)
        {
            //cout << "\nthe matrix does not exist inverse matrix\n";
            break;
        }
        // 获取列主元素
        temp = t[i][i];
        // 将主元所在的行进行单位化处理
        //cout<<"\nthe temp is "<<temp<<endl;
        for (j = 0; j < 4; j++)
        {
            t[i][j] = t[i][j] / temp;
            b[i][j] = b[i][j] / temp;
        }
        for (j = 0; j < 4; j++)
        {
            if (j != i)
            {
                temp = t[j][i];
                //消去该列的其他元素
                for (k = 0; k < 4; k++)
                {
                    t[j][k] = t[j][k] - temp * t[i][k];
                    b[j][k] = b[j][k] - temp * b[i][k];
                }
            }

        }

    }
}
void versionData()
{
     printf("20230916");
}
void multi_axis_kinematics_to_six(double* ida, double *tool, double* spacePosture)
{
    //fun = fkine()
    //*ida---jointvalue---num=5---[ida[0],ida[1],ida[2],ida[3],ida[4]]
    //*tool---tool coordinate system compensation matirx---num=3---[tool[0],tool[1],tool[2]]=[x,y,z]
    //output=*sapcePosture---terminal point coordinates---num=6---[x,y,z,R,P,Y]
    double b_a3 = 0.2415;
    double d5 = -0.1505;
    double t[3];

    t[0] = ida[2] - 0.1505 * cos(ida[3]);
    t[1] = 0.2415 + ida[0];
    t[2] = 0.1505 * sin(ida[3]) + ida[1];

    double n[3] = { 0,0,-1 };
    double o[3] = { -1,0,0 };
    double a[3] = { 0,1,0 };
    double rpy[3] = { 0,0,0 };

    n[0] = (0 - sin(ida[3]) * cos(ida[4]));
    n[1] = sin(ida[4]);
    n[2] = (0 - cos(ida[3]) * cos(ida[4]));

    o[0] = sin(ida[3]) * sin(ida[4]);
    o[1] = cos(ida[4]);
    o[2] = cos(ida[3]) * sin(ida[4]);

    a[0] = cos(ida[3]);
    a[1] = 0;
    a[2] = (0 - sin(ida[3]));

    rpy[0] = asin(n[1]);
    rpy[1] = asin(a[0]);
    rpy[2] = 0;

    //double a1 = 0;
        tool[0] = 0.529;
    //double b1 = 0;
        tool[1] = 0;
    //double c1 = 0;
        tool[2] = 0.1505;

    t[0] = n[0] * tool[0] + o[0] * tool[1] + a[0] * tool[2] + t[0];
    t[1] = n[1] * tool[0] + o[1] * tool[1] + a[1] * tool[2] + t[1];
    t[2] = n[2] * tool[0] + o[2] * tool[1] + a[2] * tool[2] + t[2];

    for (int i = 0; i < 3; i++)
    {
        spacePosture[i] = t[i];
    }
    for (int i = 3; i < 6; i++)
    {
        spacePosture[i] = rpy[i - 3];
    }
}
void calculate_vertical(double* a, double* b, double* c, double* d, double * rpy)
{
    //*a---point1---num=3---[x,y,z]
    //*b---point2---num=3---[x,y,z]
    //*c---point3---num=3---[x,y,z]
    //*d---the laser distance at three points---num=3---[d1,d2,d3]
    //output=*rpy---attitude matrix---num=3---[R,P,Y]
        double b0 = 0;
    double c0 = 0;
    b0 = b[0] + d[1] - d[0];
    c0 = c[0] + d[2] - d[0];

    double aB[3] = { 0, 0, 0 };
    double aC[3] = { 0, 0, 0 };

    aB[0] = b0 - a[0];
    aB[1] = b[1] - a[1];
    aB[2] = b[2] - a[2];
    aC[0] = c0 - a[0];
    aC[1] = c[1] - a[1];
    aC[2] = c[2] - a[2];

    double n[3] = { 0, 0, 0 };
    n[0] = aB[1] * aC[2] - aC[1] * aB[2];
    n[1] = aC[0] * aB[2] - aB[0] * aC[2];
    n[2] = aB[0] * aC[1] - aC[0] * aB[1];

    if (n[0] < 0)
    {
        n[0] = 0 - n[0];
        n[1] = 0 - n[1];
        n[2] = 0 - n[2];
    }
    else if (n[0] == 0)
    {
        n[0] = 1;
        n[1] = 0;
        n[2] = 0;
    }

    rpy[0] = asin(n[1] / (sqrt(n[0] * n[0] + n[1] * n[1])));//roll
    rpy[1] = acos(n[2] / (sqrt(n[0] * n[0] + n[2] * n[2]))) - 1.570796326794896619;//pitch
    rpy[2] = 0;
}
void calibrate_origin(double* p, double* cam_delta, double *cam_comp, double *cam, double* outp)
{
    //*p---robot coordinate---num=6---[x,y,z,R,P,Y]
    //*cam_delta---the initial parameters of the camera---num=2---[cam_x,cam_y]
    //*cam_comp---camera relative position compensation parameter matrix---num=3---[x,y,z]
    //*cam---current location camera paramters---num=2---[cam_x,cam_y]
    //*outp---the coordinates of the center of the circle taken by the camera int robot coordinate system---num=6---[x,y,z,R,P,Y]
    double T0[3][3];
    double pic[3] = { 0,0,0 };//工件坐标

    pic[0] = 0 - cam_comp[0];
    pic[1] = cam_comp[1] + cam_delta[0] - cam[0];
    pic[2] = cam_comp[2] - cam_delta[1] + cam[1];

    T0[0][0] = cos(p[4]) * cos(p[3]);
    T0[1][0] = sin(p[5]) * sin(p[4]) * cos(p[3]) + cos(p[5]) * sin(p[3]);
    T0[2][0] = sin(p[5]) * sin(p[3]) - cos(p[5]) * sin(p[4]) * cos(p[3]);

    T0[0][1] = 0 - cos(p[4]) * sin(p[3]);
    T0[1][1] = cos(p[5]) * cos(p[3]) - sin(p[5]) * sin(p[4]) * sin(p[3]);
    T0[2][1] = cos(p[5]) * sin(p[4]) * sin(p[3]) + sin(p[5]) * cos(p[3]);

    T0[0][2] = sin(p[4]);
    T0[1][2] = 0 - sin(p[5]) * cos(p[4]);
    T0[2][2] = cos(p[5]) * cos(p[4]);

    outp[0] = T0[0][0] * pic[0] + T0[0][1] * pic[1] + T0[0][2] * pic[2] + p[0];
    outp[1] = T0[1][0] * pic[0] + T0[1][1] * pic[1] + T0[1][2] * pic[2] + p[1];
    outp[2] = T0[2][0] * pic[0] + T0[2][1] * pic[1] + T0[2][2] * pic[2] + p[2];
    outp[3] = p[3];
    outp[4] = p[4];
    outp[5] = p[5];
}
void calibrate_origin2(double* p, double* cam_delta, double *cam_comp, double *cam, double *camRot, double* outp)
{
    //*camRot---camera offset compensation parameter matrix---num=2---[cam_dx,cam_dy]
    //*p---robot coordinate---num=6---[x,y,z,R,P,Y]
    //*cam_delta---the initial parameters of the camera---num=2---[cam_x,cam_y]
    //*cam_comp---camera relative position compensation parameter matrix---num=3---[x,y,z]
    //*cam---current location camera paramters---num=2---[cam_x,cam_y]
    //*outp---the coordinates of the center of the circle taken by the camera int robot coordinate system---num=6---[x,y,z,R,P,Y]

    double T0[3][3];
    double pic[3] = { 0,0,0 };//工件坐标

    pic[0] = 0 - (cam_comp[0] - cam_delta[2]);
    pic[1] = cam_comp[1] + cam_delta[0] - cam[0] + camRot[0];
    pic[2] = cam_comp[2] - cam_delta[1] + cam[1] - camRot[1];

    T0[0][0] = cos(p[4]) * cos(p[3]);
    T0[1][0] = sin(p[5]) * sin(p[4]) * cos(p[3]) + cos(p[5]) * sin(p[3]);
    T0[2][0] = sin(p[5]) * sin(p[3]) - cos(p[5]) * sin(p[4]) * cos(p[3]);

    T0[0][1] = 0 - cos(p[4]) * sin(p[3]);
    T0[1][1] = cos(p[5]) * cos(p[3]) - sin(p[5]) * sin(p[4]) * sin(p[3]);
    T0[2][1] = cos(p[5]) * sin(p[4]) * sin(p[3]) + sin(p[5]) * cos(p[3]);

    T0[0][2] = sin(p[4]);
    T0[1][2] = 0 - sin(p[5]) * cos(p[4]);
    T0[2][2] = cos(p[5]) * cos(p[4]);

    outp[0] = T0[0][0] * pic[0] + T0[0][1] * pic[1] + T0[0][2] * pic[2] + p[0];
    outp[1] = T0[1][0] * pic[0] + T0[1][1] * pic[1] + T0[1][2] * pic[2] + p[1];
    outp[2] = T0[2][0] * pic[0] + T0[2][1] * pic[1] + T0[2][2] * pic[2] + p[2];
    outp[3] = p[3];
    outp[4] = p[4];
    outp[5] = p[5];
}

void rotation_angle(double* p1, double* p2, double* outp1)
{
    //*p1---a point on the axis int robot coordinate system---num=6---[x,y,z,R,P,Y]
    //*p2---origin coordinates int robot coordinate system---num=6---[x,y,z,R,P,Y]
    //*outp1[6]---rotation_angle
    double T1[3][3];

    T1[0][0] = cos(p2[4]) * cos(p2[3]);
    T1[1][0] = sin(p2[5]) * sin(p2[4]) * cos(p2[3]) + cos(p2[5]) * sin(p2[3]);
    T1[2][0] = sin(p2[5]) * sin(p2[3]) - cos(p2[5]) * sin(p2[4]) * cos(p2[3]);

    T1[0][1] = 0 - cos(p2[4]) * sin(p2[3]);
    T1[1][1] = cos(p2[5]) * cos(p2[3]) - sin(p2[5]) * sin(p2[4]) * sin(p2[3]);
    T1[2][1] = cos(p2[5]) * sin(p2[4]) * sin(p2[3]) + sin(p2[5]) * cos(p2[3]);

    T1[0][2] = sin(p2[4]);
    T1[1][2] = 0 - sin(p2[5]) * cos(p2[4]);
    T1[2][2] = cos(p2[5]) * cos(p2[4]);

    double T2[3][3] = { 0 };
    matrix_inverse(T1, T2);

    outp1[0] = T2[0][0] * (p1[0] - p2[0]) + T2[0][1] * (p1[1] - p2[1]) + T2[0][2] * (p1[2] - p2[2]);
    outp1[1] = T2[1][0] * (p1[0] - p2[0]) + T2[1][1] * (p1[1] - p2[1]) + T2[1][2] * (p1[2] - p2[2]);
    outp1[2] = T2[2][0] * (p1[0] - p2[0]) + T2[2][1] * (p1[1] - p2[1]) + T2[2][2] * (p1[2] - p2[2]);
    outp1[3] = p2[3];
    outp1[4] = p2[4];
    outp1[5] = p2[5];

    double ss = 0;
    if (outp1[2] < 0)
    {
        if (outp1[1] <= 0)
        {
            ss = atan(abs(outp1[1] / outp1[2]));
            outp1[6] = 3.141592653589793238 - ss;
            //printf(":%f :%f \n", ss, outp1[6]);
        }
        else
        {
            ss = atan(abs(outp1[1] / outp1[2]));
            outp1[6] = -(3.141592653589793238 - ss);
        }
    }
    else if (outp1[2] > 0)
    {
        if (outp1[1] <= 0)
        {
            ss = atan(abs(outp1[1] / outp1[2]));
            outp1[6] = ss;
            //printf(":%f :%f \n", ss, outp1[6]);
        }
        else
        {
            ss = atan(abs(outp1[1] / outp1[2]));
            outp1[6] = -ss;

        }
    }
    else
    {
        if (outp1[1] > 0)
        {
            ss = -1.570796326794896619;
            outp1[6] = ss;
        }
        else if (outp1[0] < 0)
        {
            ss = 1.570796326794896619;
            outp1[6] = ss;
        }
        else
        {
            outp1[6] = 0;
        }
    }
    //printf(" :%f \n", outp1[6]);
}

void trans_2to1(double* t_pos2, double* sita, double* t_pos1)
{
    //*t_pos2---point in work2 coordinate system---num=3---[w2_x,w2_y,w2_z]
    //sita---rotation_angle
    //*t_pos1---point in work1 coordinate system---num=3---[w1_x,w1_y,w1_z]
    t_pos1[0] = t_pos2[0];
    t_pos1[1] = (cos(sita[0]) * t_pos2[1] - sin(sita[0]) * t_pos2[2]);
    t_pos1[2] = (sin(sita[0]) * t_pos2[1] + cos(sita[0]) * t_pos2[2]);
}

void workpiece_to_robot_coordinate(double* workpiece_1_origin, double* workpiece_1_position, double* w_trc_out)
{
    //workpiece_1_origin 工件1的原点在机器人坐标系里的坐标---num=6
    //workpiece_1_position 工件1的坐标值---num=3
    //w_trc_out---robot coordinates---num=6
    double w_trc_T0[3] = { 0, 0, 0 };
    double w_trc_T1[3] = { 0, 0, 0 };
    double w_trc_T2[3] = { 0, 0, 0 };

    w_trc_T0[0] = cos(workpiece_1_origin[4]) * cos(workpiece_1_origin[3]);
    w_trc_T1[0] = sin(workpiece_1_origin[5]) * sin(workpiece_1_origin[4])*cos(workpiece_1_origin[3]) + cos(workpiece_1_origin[5]) * sin(workpiece_1_origin[3]);
    w_trc_T2[0] = sin(workpiece_1_origin[5]) * sin(workpiece_1_origin[3]) - cos(workpiece_1_origin[5]) * sin(workpiece_1_origin[4]) * cos(workpiece_1_origin[3]);

    w_trc_T0[1] = 0 - cos(workpiece_1_origin[4]) * sin(workpiece_1_origin[3]);
    w_trc_T1[1] = cos(workpiece_1_origin[5]) * cos(workpiece_1_origin[3]) - sin(workpiece_1_origin[5]) * sin(workpiece_1_origin[4]) * sin(workpiece_1_origin[3]);
    w_trc_T2[1] = cos(workpiece_1_origin[5]) * sin(workpiece_1_origin[4]) * sin(workpiece_1_origin[3]) + sin(workpiece_1_origin[5]) * cos(workpiece_1_origin[3]);

    w_trc_T0[2] = sin(workpiece_1_origin[4]);
    w_trc_T1[2] = 0 - sin(workpiece_1_origin[5]) * cos(workpiece_1_origin[4]);
    w_trc_T2[2] = cos(workpiece_1_origin[5]) * cos(workpiece_1_origin[4]);

    w_trc_out[0] = w_trc_T0[0] * workpiece_1_position[0] + w_trc_T0[1] * workpiece_1_position[1] + w_trc_T0[2] * workpiece_1_position[2] + workpiece_1_origin[0];
    w_trc_out[1] = w_trc_T1[0] * workpiece_1_position[0] + w_trc_T1[1] * workpiece_1_position[1] + w_trc_T1[2] * workpiece_1_position[2] + workpiece_1_origin[1];
    w_trc_out[2] = w_trc_T2[0] * workpiece_1_position[0] + w_trc_T2[1] * workpiece_1_position[1] + w_trc_T2[2] * workpiece_1_position[2] + workpiece_1_origin[2];
    w_trc_out[3] = workpiece_1_origin[3];
    w_trc_out[4] = workpiece_1_origin[4];
    w_trc_out[5] = workpiece_1_origin[5];
}

void robotTo_workpiece2(double* robot_space_pos, double* s_wp2_origin, double* sita, double* s_wp2_pos)
{
    //robot_space_pos---robot coordinates---num=6
    // s_wp2_origin[x ,y, z, R, P, Y]原点坐标
    // sita 求出的旋转角度
    //*s_wp2_pos---point int work2 coordinate system---num=6
    double s_wp2_T1[3][3] = { 0 };
    double s_wp2_T2[3][3] = { 0 };
    // p机器人原点坐标 xyz rpy

    s_wp2_T1[0][0] = cos(s_wp2_origin[4]) * cos(s_wp2_origin[3]);
    s_wp2_T1[1][0] = sin(s_wp2_origin[5]) * sin(s_wp2_origin[4]) * cos(s_wp2_origin[3]) + cos(s_wp2_origin[5]) * sin(s_wp2_origin[3]);
    s_wp2_T1[2][0] = sin(s_wp2_origin[5]) * sin(s_wp2_origin[3]) - cos(s_wp2_origin[5]) * sin(s_wp2_origin[4]) * cos(s_wp2_origin[3]);

    s_wp2_T1[0][1] = 0 - cos(s_wp2_origin[4]) * sin(s_wp2_origin[3]);
    s_wp2_T1[1][1] = cos(s_wp2_origin[5]) * cos(s_wp2_origin[3]) - sin(s_wp2_origin[5]) * sin(s_wp2_origin[4]) * sin(s_wp2_origin[3]);
    s_wp2_T1[2][1] = cos(s_wp2_origin[5]) * sin(s_wp2_origin[4]) * sin(s_wp2_origin[3]) + sin(s_wp2_origin[5]) * cos(s_wp2_origin[3]);

    s_wp2_T1[0][2] = sin(s_wp2_origin[4]);
    s_wp2_T1[1][2] = 0 - sin(s_wp2_origin[5]) * cos(s_wp2_origin[4]);
    s_wp2_T1[2][2] = cos(s_wp2_origin[5]) * cos(s_wp2_origin[4]);

    // 求逆矩阵
    matrix_inverse(s_wp2_T1, s_wp2_T2);

    // 工件1 坐标
    double s_wp1[6] = { 0 };
    s_wp1[0] = s_wp2_T2[0][0] * (robot_space_pos[0] - s_wp2_origin[0]) + s_wp2_T2[0][1] * (robot_space_pos[1] - s_wp2_origin[1]) + s_wp2_T2[0][2] * (robot_space_pos[2] - s_wp2_origin[2]);
    s_wp1[1] = s_wp2_T2[1][0] * (robot_space_pos[0] - s_wp2_origin[0]) + s_wp2_T2[1][1] * (robot_space_pos[1] - s_wp2_origin[1]) + s_wp2_T2[1][2] * (robot_space_pos[2] - s_wp2_origin[2]);
    s_wp1[2] = s_wp2_T2[2][0] * (robot_space_pos[0] - s_wp2_origin[0]) + s_wp2_T2[2][1] * (robot_space_pos[1] - s_wp2_origin[1]) + s_wp2_T2[2][2] * (robot_space_pos[2] - s_wp2_origin[2]);
    s_wp1[3] = robot_space_pos[3];
    s_wp1[4] = robot_space_pos[4];
    s_wp1[5] = robot_space_pos[5];

    // 工件2 坐标
    s_wp2_pos[0] = s_wp1[0];
    s_wp2_pos[1] = (cos(sita[0]) * s_wp1[1] + sin(sita[0]) * s_wp1[2]);
    s_wp2_pos[2] = (cos(sita[0]) * s_wp1[2] - sin(sita[0]) * s_wp1[1]);
}

void trans_to_workpiece1(double* origin_pos, double* robot_space_pos, double* s_wp1)
{
    // s_wp2_origin[x ,y, z, R, P, Y]原点坐标
    //robot_space_pos---robot coordinates---num=6
    //*s_wp1---point int work1 coordinate system---num=6
    double s_wp2_T1[3][3] = { 0 };
    double s_wp2_T2[3][3] = { 0 };
    // p机器人原点坐标 xyz rpy

    s_wp2_T1[0][0] = cos(origin_pos[4]) * cos(origin_pos[3]);
    s_wp2_T1[1][0] = sin(origin_pos[5]) * sin(origin_pos[4]) * cos(origin_pos[3]) + cos(origin_pos[5]) * sin(origin_pos[3]);
    s_wp2_T1[2][0] = sin(origin_pos[5]) * sin(origin_pos[3]) - cos(origin_pos[5]) * sin(origin_pos[4]) * cos(origin_pos[3]);

    s_wp2_T1[0][1] = 0 - cos(origin_pos[4]) * sin(origin_pos[3]);
    s_wp2_T1[1][1] = cos(origin_pos[5]) * cos(origin_pos[3]) - sin(origin_pos[5]) * sin(origin_pos[4]) * sin(origin_pos[3]);
    s_wp2_T1[2][1] = cos(origin_pos[5]) * sin(origin_pos[4]) * sin(origin_pos[3]) + sin(origin_pos[5]) * cos(origin_pos[3]);

    s_wp2_T1[0][2] = sin(origin_pos[4]);
    s_wp2_T1[1][2] = 0 - sin(origin_pos[5]) * cos(origin_pos[4]);
    s_wp2_T1[2][2] = cos(origin_pos[5]) * cos(origin_pos[4]);

    // 求逆矩阵
    matrix_inverse(s_wp2_T1, s_wp2_T2);

    // 工件1 坐标
    s_wp1[0] = s_wp2_T2[0][0] * (robot_space_pos[0] - origin_pos[0]) + s_wp2_T2[0][1] * (robot_space_pos[1] - origin_pos[1]) + s_wp2_T2[0][2] * (robot_space_pos[2] - origin_pos[2]);
    s_wp1[1] = s_wp2_T2[1][0] * (robot_space_pos[0] - origin_pos[0]) + s_wp2_T2[1][1] * (robot_space_pos[1] - origin_pos[1]) + s_wp2_T2[1][2] * (robot_space_pos[2] - origin_pos[2]);
    s_wp1[2] = s_wp2_T2[2][0] * (robot_space_pos[0] - origin_pos[0]) + s_wp2_T2[2][1] * (robot_space_pos[1] - origin_pos[1]) + s_wp2_T2[2][2] * (robot_space_pos[2] - origin_pos[2]);
    //s_wp1[3] = robot_space_pos[3];
    //s_wp1[4] = robot_space_pos[4];
    //s_wp1[5] = robot_space_pos[5];

}

void adjust_matrix(double* p1, double* p2, double* p_matrix)
{
    //gongjian1dianwei:
    //# p1枪位置 对应激光距离l1
    //# p2相机位置 对应激光距离l2
    //*p_matrix---hand-eye calibration compensation matrix---num=3---[dx,dy,dz]
    p_matrix[0] = p2[0] - p1[0];
    p_matrix[1] = p1[1] - p2[1];
    p_matrix[2] = p1[2] - p2[2];
}

void camera_to_robot_trans(double* p_matrix, double* camera, double* camera_pos, double* robot_pos)
{
    //# p_matrix---位置变换关系矩阵
    //*camera---the parameters of camer---num=2---[cam_x,cam_y]
    //*camera_pos---robot coordinates when camera takes picture---num=6
    //* robot_pos---robot coordinates when torch arrived---num=6
    double pic[3] = { 0 };
    double c_to_r_T[3][3] = { 0 };
    //# 工件坐标
    pic[0] = p_matrix[0];
    pic[1] = 0 - p_matrix[1] - camera[0];
    pic[2] = 0 - p_matrix[2] + camera[1];

    c_to_r_T[0][0] = cos(camera_pos[4]) * cos(camera_pos[3]);
    c_to_r_T[1][0] = sin(camera_pos[5]) * sin(camera_pos[4]) * cos(camera_pos[3]) + cos(camera_pos[5]) * sin(camera_pos[3]);
    c_to_r_T[2][0] = sin(camera_pos[5]) * sin(camera_pos[3]) - cos(camera_pos[5]) * sin(camera_pos[4]) * cos(camera_pos[3]);

    c_to_r_T[0][1] = 0 - cos(camera_pos[4]) * sin(camera_pos[3]);
    c_to_r_T[1][1] = cos(camera_pos[5]) * cos(camera_pos[3]) - sin(camera_pos[5]) * sin(camera_pos[4]) * sin(camera_pos[3]);
    c_to_r_T[2][1] = cos(camera_pos[5]) * sin(camera_pos[4]) * sin(camera_pos[3]) + sin(camera_pos[5]) * cos(camera_pos[3]);

    c_to_r_T[0][2] = sin(camera_pos[4]);
    c_to_r_T[1][2] = 0 - sin(camera_pos[5]) * cos(camera_pos[4]);
    c_to_r_T[2][2] = cos(camera_pos[5]) * cos(camera_pos[4]);

    robot_pos[0] = c_to_r_T[0][0] * pic[0] + c_to_r_T[0][1] * pic[1] + c_to_r_T[0][2] * pic[2] + camera_pos[0];
    robot_pos[1] = c_to_r_T[1][0] * pic[0] + c_to_r_T[1][1] * pic[1] + c_to_r_T[1][2] * pic[2] + camera_pos[1];
    robot_pos[2] = c_to_r_T[2][0] * pic[0] + c_to_r_T[2][1] * pic[1] + c_to_r_T[2][2] * pic[2] + camera_pos[2];
    robot_pos[3] = camera_pos[3];
    robot_pos[4] = camera_pos[4];
    robot_pos[5] = camera_pos[5];
}

void laser_to_robot_trans(double* lp_matrix, double* laser_pos, double* robot_pos)
{
    //# lp_matrix---位置变换关系矩阵
    //*laser_pos---robot coordinates when laser is---num=6
    //* robot_pos---robot coordinates when torch arrived---num=6
    double pic[3] = { 0 };
    double c_to_r_T[3][3] = { 0 };
    //# 工件坐标
    pic[0] = lp_matrix[0];
    pic[1] = 0 - lp_matrix[1];
    pic[2] = 0 - lp_matrix[2];

    c_to_r_T[0][0] = cos(laser_pos[4]) * cos(laser_pos[3]);
    c_to_r_T[1][0] = sin(laser_pos[5]) * sin(laser_pos[4]) * cos(laser_pos[3]) + cos(laser_pos[5]) * sin(laser_pos[3]);
    c_to_r_T[2][0] = sin(laser_pos[5]) * sin(laser_pos[3]) - cos(laser_pos[5]) * sin(laser_pos[4]) * cos(laser_pos[3]);

    c_to_r_T[0][1] = 0 - cos(laser_pos[4]) * sin(laser_pos[3]);
    c_to_r_T[1][1] = cos(laser_pos[5]) * cos(laser_pos[3]) - sin(laser_pos[5]) * sin(laser_pos[4]) * sin(laser_pos[3]);
    c_to_r_T[2][1] = cos(laser_pos[5]) * sin(laser_pos[4]) * sin(laser_pos[3]) + sin(laser_pos[5]) * cos(laser_pos[3]);

    c_to_r_T[0][2] = sin(laser_pos[4]);
    c_to_r_T[1][2] = 0 - sin(laser_pos[5]) * cos(laser_pos[4]);
    c_to_r_T[2][2] = cos(laser_pos[5]) * cos(laser_pos[4]);

    robot_pos[0] = c_to_r_T[0][0] * pic[0] + c_to_r_T[0][1] * pic[1] + c_to_r_T[0][2] * pic[2] + laser_pos[0];
    robot_pos[1] = c_to_r_T[1][0] * pic[0] + c_to_r_T[1][1] * pic[1] + c_to_r_T[1][2] * pic[2] + laser_pos[1];
    robot_pos[2] = c_to_r_T[2][0] * pic[0] + c_to_r_T[2][1] * pic[1] + c_to_r_T[2][2] * pic[2] + laser_pos[2];
    robot_pos[3] = laser_pos[3];
    robot_pos[4] = laser_pos[4];
    robot_pos[5] = laser_pos[5];
}

void robot_to_laser_trans(double* lp_matrix, double* robot_pos, double* laser_pos)
{
    //# lp_matrix---位置变换关系矩阵
    //*robot_pos---robot coordinates when laser is---num=6
    //* laser_pos---robot coordinates when torch arrived---num=6
    double pic[3] = { 0 };
    double c_to_r_T[3][3] = { 0 };
    //# 工件坐标
    pic[0] = 0 - lp_matrix[0];
    pic[1] = lp_matrix[1];
    pic[2] = lp_matrix[2];

    c_to_r_T[0][0] = cos(robot_pos[4]) * cos(robot_pos[3]);
    c_to_r_T[1][0] = sin(robot_pos[5]) * sin(robot_pos[4]) * cos(robot_pos[3]) + cos(robot_pos[5]) * sin(robot_pos[3]);
    c_to_r_T[2][0] = sin(robot_pos[5]) * sin(robot_pos[3]) - cos(robot_pos[5]) * sin(robot_pos[4]) * cos(robot_pos[3]);

    c_to_r_T[0][1] = 0 - cos(robot_pos[4]) * sin(robot_pos[3]);
    c_to_r_T[1][1] = cos(robot_pos[5]) * cos(robot_pos[3]) - sin(robot_pos[5]) * sin(robot_pos[4]) * sin(robot_pos[3]);
    c_to_r_T[2][1] = cos(robot_pos[5]) * sin(robot_pos[4]) * sin(robot_pos[3]) + sin(robot_pos[5]) * cos(robot_pos[3]);

    c_to_r_T[0][2] = sin(robot_pos[4]);
    c_to_r_T[1][2] = 0 - sin(robot_pos[5]) * cos(robot_pos[4]);
    c_to_r_T[2][2] = cos(robot_pos[5]) * cos(robot_pos[4]);

    laser_pos[0] = c_to_r_T[0][0] * pic[0] + c_to_r_T[0][1] * pic[1] + c_to_r_T[0][2] * pic[2] + robot_pos[0];
    laser_pos[1] = c_to_r_T[1][0] * pic[0] + c_to_r_T[1][1] * pic[1] + c_to_r_T[1][2] * pic[2] + robot_pos[1];
    laser_pos[2] = c_to_r_T[2][0] * pic[0] + c_to_r_T[2][1] * pic[1] + c_to_r_T[2][2] * pic[2] + robot_pos[2];
    laser_pos[3] = robot_pos[3];
    laser_pos[4] = robot_pos[4];
    laser_pos[5] = robot_pos[5];
}

void cad_to_work2_trans_matrix(double* p_a0, double* p_b0, double* p_a1, double* p_b1, double* solution)
{
    //# p_a0 工件2：机器人坐标[x0, y0, z0]---对应---p_b0 cad坐标[x1, y1]
    //# p_a1 工件2：机器人坐标[x2, y2, z2]---对应---p_b1 cad坐标[x3, y3]
    //# cad_p cad坐标
    //# print(' 机器人点1' + str(p_a0))
    //# print(' cad点1' + str(p_b0))
    //# print(' 机器人点2' + str(p_a1))
    //# print(' cad点2' + str(p_b1))
        //m = np.array([[1, 0, p_b0[0], -p_b0[1]], [0, 1, p_b0[1], p_b0[0]], [1, 0, p_b1[0], -p_b1[1]], [0, 1, p_b1[1], p_b1[0]]])
        //n = np.array([p_a0[2], p_a0[1], p_a1[2], p_a1[1]])

        double arr_L[4][4] = { 0 };
    double arr_R[4][4] = { 0 };
    arr_L[0][0] = 1;
    arr_L[0][1] = 0;
    arr_L[0][2] = p_b0[0];
    arr_L[0][3] = -p_b0[1];

    arr_L[1][0] = 0;
    arr_L[1][1] = 1;
    arr_L[1][2] = p_b0[1];
    arr_L[1][3] = p_b0[0];

    arr_L[2][0] = 1;
    arr_L[2][1] = 0;
    arr_L[2][2] = p_b1[0];
    arr_L[2][3] = -p_b1[1];

    arr_L[3][0] = 0;
    arr_L[3][1] = 1;
    arr_L[3][2] = p_b1[1];
    arr_L[3][3] = p_b1[0];

    matrix_inverse4(arr_L, arr_R);

    solution[0] = arr_R[0][0]*p_a0[2]+arr_R[0][1]*p_a0[1]+arr_R[0][2]*p_a1[2]+arr_R[0][3]*p_a1[1];
    solution[1] = arr_R[1][0]*p_a0[2]+arr_R[1][1]*p_a0[1]+arr_R[1][2]*p_a1[2]+arr_R[1][3]*p_a1[1];
    solution[2] = arr_R[2][0]*p_a0[2]+arr_R[2][1]*p_a0[1]+arr_R[2][2]*p_a1[2]+arr_R[2][3]*p_a1[1];
    solution[3] = arr_R[3][0]*p_a0[2]+arr_R[3][1]*p_a0[1]+arr_R[3][2]*p_a1[2]+arr_R[3][3]*p_a1[1];
    //# print('旋转矩阵')
    //# print(solution)
}


void cad_to_work2_coordinate(double* cad_pos, double* trans_matrix, double* deep, double* w2_pos)
{
    //# cad_pos cad坐标
    //# trans_matrix 变换矩阵
    //# print(' cad坐标' + str(cad_pos))
    w2_pos[0] = deep[0];
    w2_pos[1] = trans_matrix[3] * cad_pos[0] + trans_matrix[2] * cad_pos[1] + trans_matrix[1];
    w2_pos[2] = trans_matrix[2] * cad_pos[0] - trans_matrix[3] * cad_pos[1] + trans_matrix[0];
    //# w2_pos [x, y, z]工件2坐标
}

void gt_to_work2_trans_matrix(double* p_a0, double* p_b0, double* p_a1, double* p_b1, double* solution)
{
    //# p_a0 工件2：机器人坐标[z0, y0]---对应---p_b0 gt坐标[x1, y1]
    //# p_a1 工件2：机器人坐标[z2, y2]---对应---p_b1 gt坐标[x3, y3]
    double arr_L[4][4] = { 0 };
    double arr_R[4][4] = { 0 };
    arr_L[0][0] = 1;
    arr_L[0][1] = 0;
    arr_L[0][2] = p_b0[0];
    arr_L[0][3] = -p_b0[1];

    arr_L[1][0] = 0;
    arr_L[1][1] = 1;
    arr_L[1][2] = p_b0[1];
    arr_L[1][3] = p_b0[0];

    arr_L[2][0] = 1;
    arr_L[2][1] = 0;
    arr_L[2][2] = p_b1[0];
    arr_L[2][3] = -p_b1[1];

    arr_L[3][0] = 0;
    arr_L[3][1] = 1;
    arr_L[3][2] = p_b1[1];
    arr_L[3][3] = p_b1[0];

    matrix_inverse4(arr_L, arr_R);

    solution[0] = arr_R[0][0] * p_a0[1] + arr_R[0][1] * p_a0[0] + arr_R[0][2] * p_a1[1] + arr_R[0][3] * p_a1[0];
    solution[1] = arr_R[1][0] * p_a0[1] + arr_R[1][1] * p_a0[0] + arr_R[1][2] * p_a1[1] + arr_R[1][3] * p_a1[0];
    solution[2] = arr_R[2][0] * p_a0[1] + arr_R[2][1] * p_a0[0] + arr_R[2][2] * p_a1[1] + arr_R[2][3] * p_a1[0];
    solution[3] = arr_R[3][0] * p_a0[1] + arr_R[3][1] * p_a0[0] + arr_R[3][2] * p_a1[1] + arr_R[3][3] * p_a1[0];
}


void gt_to_work2_coordinate(double* gt_pos, double* trans_matrix, double* w2_pos)
{
    w2_pos[0] = trans_matrix[3] * gt_pos[0] + trans_matrix[2] * gt_pos[1] + trans_matrix[1];
    w2_pos[1] = trans_matrix[2] * gt_pos[0] - trans_matrix[3] * gt_pos[1] + trans_matrix[0];
    //# w2_pos [y, z]工件2坐标
    //printf("y = %f\t z = %f \n", w2_pos[0], w2_pos[1]);
}
