#include "libmanager.h"

void motordecToCJointpos(int currentaxisdec[5], motorparam_ mp[5], double *joint)
{
    // 功能：dec->关节坐标
    // int currentaxisdec[5]:当前的轴dec
    // motorparam_ mp[5]:电机传动参数
    // double *joint:返回值 关节坐标joint[0]-[4]
    joint[0] = ((double)(currentaxisdec[0] * mp[0].direction - mp[0].initDec) /
                (mp[0].transmission * 10000.0));
    joint[1] = ((double)(currentaxisdec[1] * mp[1].direction - mp[1].initDec) /
                (mp[1].transmission * 10000.0));
    joint[2] = ((double)(currentaxisdec[2] * mp[2].direction - mp[2].initDec) /
                (mp[2].transmission * 10000.0));
    joint[3] = ((double)(currentaxisdec[3] * mp[3].direction - mp[3].initDec) /
                (mp[3].transmission * 10000.0));
    joint[4] = ((double)(currentaxisdec[4] * mp[4].direction - mp[4].initDec) /
                (mp[4].transmission * 10000.0));
}

void baseToSixEndlink(double *ida, double *spacePosture)
{
    // base关节坐标->世界坐标Endlink
    // double *ida:关节坐标 [0]-[4]
    // double *spacePosture:世界坐标 [0]-[5]
    double t[3];

    t[0] = ida[2] - 0.1505 * cos(ida[3]);
    t[1] = 0.2415 + ida[0];
    t[2] = 0.1505 * sin(ida[3]) + ida[1];

    double n[3] = {0, 0, -1};
    double o[3] = {-1, 0, 0};
    double a[3] = {0, 1, 0};
    double rpy[3] = {0, 0, 0};

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

    double al = 0.529;
    double bl = 0;
    double cl = 0.1505;

    t[0] = n[0] * al + o[0] * bl + a[0] * cl + t[0];
    t[1] = n[1] * al + o[1] * bl + a[1] * cl + t[1];
    t[2] = n[2] * al + o[2] * bl + a[2] * cl + t[2];

    for (int i = 0; i < 3; i++)
    {
        spacePosture[i] = t[i];
    }
    for (int i = 3; i < 6; i++)
    {
        spacePosture[i] = rpy[i - 3];
    }
    // printf("---------------------------------------------------\n");
    // printf("%f\t %f\t %f \n", spacePosture[0], spacePosture[1], spacePosture[2]);
    // printf("%f\t %f\t %f \n", spacePosture[3], spacePosture[4], spacePosture[5]);
    // printf("---------------------------------------------------\n");
}

void sixToBaseEndlink(double endx, double endy, double endz, double r, double p, double y, double *axisdec)
{
    // 世界坐标Endlink -> base关节坐标
    // double endx, double endy, double endz, double r, double p, double y:世界坐标 x,y,z,or,op,oy
    // double *axisdec:关节坐标 [0]-[4]
    double n[3] = {0, 0, -1};
    double o[3] = {-1, 0, 0};
    double a[3] = {0, 1, 0};

    double t[3] = {0, 0, 0};
    // double rpy[3] = {0, 0, 0};

    n[0] = cos(p) * cos(r);
    n[1] = sin(y) * sin(p) * cos(r) + cos(y) * sin(r);
    n[2] = sin(y) * sin(r) - cos(y) * sin(p) * cos(r);

    o[0] = 0 - cos(p) * sin(r);
    o[1] = cos(y) * cos(r) - sin(y) * sin(p) * sin(r);
    o[2] = cos(y) * sin(p) * sin(r) + sin(y) * cos(r);

    a[0] = sin(p);
    a[1] = 0 - sin(y) * cos(p);
    a[2] = cos(y) * cos(p);

    double al = 0.529;
    double bl = 0;
    double cl = 0.1505;

    t[0] = n[0] * (0 - al) + o[0] * (0 - bl) + a[0] * (0 - cl) + endx;
    t[1] = n[1] * (0 - al) + o[1] * (0 - bl) + a[1] * (0 - cl) + endy;
    t[2] = n[2] * (0 - al) + o[2] * (0 - bl) + a[2] * (0 - cl) + endz;

    double b_a3 = 0.2415;
    // double d5 = -0.1505;

    axisdec[0] = (t[1] - b_a3);
    axisdec[3] = (0 - acos(a[0]));
    axisdec[1] = (t[2] - 0.1505 * sin(axisdec[3]));
    axisdec[2] = (t[0] + 0.1505 * cos(axisdec[3]));
    axisdec[4] = (asin(n[1]));
    /*
        for (int i = 0; i < 5; i++)
        {
            printf("sixtoendlink%f \n", axisdec[i]);
        }
    */
}
int sinefittingPlan(int step, double stoptime, int initVel_i, double circleTime, double tm, double td, double vMax, double targetposition, int *nowp, int *nowvel)
{
    // int ,int ,double ,int ,int ,int ,int ,int ,int

    double initVel;

    initVel = (double)(initVel_i);

    double ctemp = 300.0;
    ctemp = stoptime;
    double td2;
    double ttd2;
    double decMax;
    double accMax;
    double t1, t2, t3;
    double s1, s2, s3;
    double tt1;

    if (fabs(tm) < 0.01 || fabs(td) < 0.01)
    {
        tm = ctemp;
        td = ctemp;
    }
    else
    {
    }
    // printf("tm = %f\ttd = %f\n",tm,td);

    td2 = ctemp;
    ttd2 = 0.001 * ctemp;
    double stepTime = (double)(step)*circleTime;
    double stepTime2;
    stepTime2 = stepTime * stepTime;
    if (fabs(vMax) < 0.01)
    {
        stepTime += circleTime;
        stepTime2 = stepTime * stepTime;
        if (!initVel)
            return 1;
        decMax = 1.5 * (initVel - vMax) / ttd2;
        if (stepTime <= ttd2)
        {
            // printf("decMax = %f\ttd2 = %f\n",decMax,td2);
            *nowp = (int)(initVel * (stepTime)-2.0 * decMax * ((stepTime2 * stepTime / (td2 * 0.003) - stepTime2 * stepTime2 / (td2 * td2 * 0.000006))));
            *nowvel = (int)(initVel - 4.0 * decMax * (stepTime2 / (td2 * 0.002) - stepTime2 * stepTime / (td2 * td2 * 0.000003)));
            // printf("STOP:nowp = %d\tnovel = %d\n",nowp,nowvel);
        }
        else
        {
            return 1;
        }
    }
    else
    {
        accMax = 1.5 * (vMax - initVel) / (0.001 * tm);
        decMax = 1.5 * (vMax - 0) / (0.001 * td);
        t1 = 0.001 * tm;
        t2 = 3.0;
        t3 = 0.001 * td;

        s1 = fabs(initVel * t1 + 2.0 * accMax * (t1 * t1 / 3.0 - t1 * t1 / 6.0));
        s3 = fabs(vMax * t3 - 2.0 * decMax * (t3 * t3 / 3.0 - t3 * t3 / 6.0));
        s2 = targetposition - s1 - s3;

        for (int i = 0; i < 5; i++)
        {
            if (s2 < 0)
            {
                vMax = 0.5 * vMax;
                accMax = 1.5 * (vMax - initVel) / (0.001 * tm);
                decMax = 1.5 * (vMax) / (0.001 * td);
                t1 = 0.001 * tm;
                t2 = 3.0;
                t3 = 0.001 * td;
                s1 = fabs(initVel * t1 + 2.0 * accMax * (t1 * t1 / 3.0 - t1 * t1 / 6.0));
                s3 = fabs(vMax * t3 - 2.0 * decMax * (t3 * t3 / 3.0 - t3 * t3 / 6.0));
                s2 = targetposition - s1 - s3;
            }
            else
            {
                i = 10;
            }
        }
        t2 = s2 / fabs(vMax) + t1;
        t3 = t2 + t3;
        tt1 = t1 * t1;

        // printf("t1=%f t2=%f t3=%f \n",t1,t2,t3);
        // printf("s1=%f s2=%f s3=%f \n",s1,s2,s3);
        // printf("accMax=%f decMax=%f\n",accMax,decMax);
        if (vMax)
        {
            double planDistance = s1 + abs(vMax) * (t2 - t1) + s3;
            if (abs(planDistance) > targetposition + 1)
            {
                return 1;
            }
            else
            {
                if (stepTime <= t1)
                {
                    *nowp = (int)(initVel * stepTime + 2.0 * accMax * (stepTime * stepTime * stepTime / (t1 * 3.0) - stepTime * stepTime * stepTime * stepTime / (tt1 * 6.0)));
                    *nowvel = (int)(initVel + 2.0 * accMax * stepTime * stepTime / t1 - 4.0 * accMax * stepTime * stepTime * stepTime / (tt1 * 3.0));
                    // printf("ADD:nowp = %d\tnovel = %d\n",nowp,nowvel);
                }
                else if (stepTime <= t2)
                {
                    *nowp = (int)(initVel * t1 + accMax * tt1 / 3.0 + vMax * (stepTime - t1));
                    *nowvel = (int)(vMax);
                    // printf("MAX:nowp = %d\tnovel = %d\n",nowp,nowvel);
                }
                else if (stepTime <= t3)
                {
                    *nowp = (int)(initVel * t1 + accMax * tt1 / 3.0 + vMax * (t2 - t1) + vMax * (stepTime - t2) - 2.0 * decMax * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) / (t1 * 3.0) + decMax * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) / (tt1 * 3.0));
                    *nowvel = (int)(vMax - 2.0 * decMax * (stepTime - t2) * (stepTime - t2) / t1 + 4.0 * decMax * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) / (tt1 * 3.0));
                    // printf("DEC:nowp = %d\tnovel = %d\n",nowp,nowvel);
                }
                else
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}
int sinefittingPlan_movl(int step, double stoptime, double initVel_i, double circleTime, double tm, double td, double vMax, double targetposition, double *nowp, double *nowvel)
{
    // int ,int ,double ,int ,int ,int ,int ,int ,int

    double initVel;

    initVel = (double)(initVel_i);

    double ctemp = 300.0;
    ctemp = stoptime;
    double td2;
    double ttd2;
    double decMax;
    double accMax;
    double t1, t2, t3;
    double s1, s2, s3;
    double tt1;

    if (fabs(tm) < 0.01 || fabs(td) < 0.01)
    {
        tm = ctemp;
        td = ctemp;
    }
    else
    {
    }
    // printf("tm = %f\ttd = %f\n",tm,td);

    td2 = ctemp;
    ttd2 = 0.001 * ctemp;
    double stepTime = (double)(step)*circleTime;
    double stepTime2;
    stepTime2 = stepTime * stepTime;
    if (fabs(vMax) < 0.01)
    {
        stepTime += circleTime;
        stepTime2 = stepTime * stepTime;
        if (initVel < 1e-9)
            return 1;
        decMax = 1.5 * (initVel - vMax) / ttd2;
        if (stepTime <= ttd2)
        {
            // printf("decMax = %f\ttd2 = %f\n",decMax,td2);
            *nowp = (initVel * (stepTime)-2.0 * decMax * ((stepTime2 * stepTime / (td2 * 0.003) - stepTime2 * stepTime2 / (td2 * td2 * 0.000006))));
            *nowvel = (initVel - 4.0 * decMax * (stepTime2 / (td2 * 0.002) - stepTime2 * stepTime / (td2 * td2 * 0.000003)));
            // printf("STOP:nowp = %d\tnovel = %d\n",nowp,nowvel);
        }
        else
        {
            return 1;
        }
    }
    else
    {
        accMax = 1.5 * (vMax - initVel) / (0.001 * tm);
        decMax = 1.5 * (vMax - 0) / (0.001 * td);
        t1 = 0.001 * tm;
        t2 = 3.0;
        t3 = 0.001 * td;

        s1 = fabs(initVel * t1 + 2.0 * accMax * (t1 * t1 / 3.0 - t1 * t1 / 6.0));
        s3 = fabs(vMax * t3 - 2.0 * decMax * (t3 * t3 / 3.0 - t3 * t3 / 6.0));
        s2 = targetposition - s1 - s3;

        for (int i = 0; i < 5; i++)
        {
            if (s2 < 0)
            {
                vMax = 0.5 * vMax;
                accMax = 1.5 * (vMax - initVel) / (0.001 * tm);
                decMax = 1.5 * (vMax) / (0.001 * td);
                t1 = 0.001 * tm;
                t2 = 3.0;
                t3 = 0.001 * td;
                s1 = fabs(initVel * t1 + 2.0 * accMax * (t1 * t1 / 3.0 - t1 * t1 / 6.0));
                s3 = fabs(vMax * t3 - 2.0 * decMax * (t3 * t3 / 3.0 - t3 * t3 / 6.0));
                s2 = targetposition - s1 - s3;
            }
            else
            {
                i = 10;
            }
        }
        t2 = s2 / fabs(vMax) + t1;
        t3 = t2 + t3;
        tt1 = t1 * t1;

        // printf("t1=%f t2=%f t3=%f \n",t1,t2,t3);
        // printf("s1=%f s2=%f s3=%f \n",s1,s2,s3);
        // printf("accMax=%f decMax=%f\n",accMax,decMax);
        if (vMax)
        {
            double planDistance = s1 + abs(vMax) * (t2 - t1) + s3;
            if (abs(planDistance) > targetposition + 1)
            {
                return 1;
            }
            else
            {
                if (stepTime <= t1)
                {
                    *nowp = (initVel * stepTime + 2.0 * accMax * (stepTime * stepTime * stepTime / (t1 * 3.0) - stepTime * stepTime * stepTime * stepTime / (tt1 * 6.0)));
                    *nowvel = (initVel + 2.0 * accMax * stepTime * stepTime / t1 - 4.0 * accMax * stepTime * stepTime * stepTime / (tt1 * 3.0));
                    // printf("ADD:nowp = %d\tnovel = %d\n",nowp,nowvel);
                }
                else if (stepTime <= t2)
                {
                    *nowp = (initVel * t1 + accMax * tt1 / 3.0 + vMax * (stepTime - t1));
                    *nowvel = (vMax);
                    // printf("MAX:nowp = %d\tnovel = %d\n",nowp,nowvel);
                }
                else if (stepTime <= t3)
                {
                    *nowp = (initVel * t1 + accMax * tt1 / 3.0 + vMax * (t2 - t1) + vMax * (stepTime - t2) - 2.0 * decMax * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) / (t1 * 3.0) + decMax * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) / (tt1 * 3.0));
                    *nowvel = (vMax - 2.0 * decMax * (stepTime - t2) * (stepTime - t2) / t1 + 4.0 * decMax * (stepTime - t2) * (stepTime - t2) * (stepTime - t2) / (tt1 * 3.0));
                    // printf("DEC:nowp = %d\tnovel = %d\n",nowp,nowvel);
                }
                else
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}
int trapezoidalPlanOnceTime_stopTime(int step, double stoptime, int initVel, double circleTime, double accTime, double decTime, double v, double targetposition, int *nowp, int *nowvel)
{
    // 轨迹插补算法
    // int step:计数
    // double stoptime:停止时间
    // int initVel:初始速度
    // double circleTime:循环周期
    // double accTime,double decTime,double v,double targetposition:插补参数
    // int *nowp,int *nowvel:返回参数：当前步距，当前速度
    // return 0 正常计算;1 停止计算;
    PRINTF("step=%d, stoptime=%f, initVel=%d, circleTime=%f, accTime=%f, decTime=%f, v=%f, targetposition=%f;\r\n",
           step, stoptime, initVel, circleTime, accTime, decTime, v, targetposition);
    double stepTime = (double)step * circleTime;
    accTime *= 0.001;
    decTime *= 0.001;
    if (v)
    {
        targetposition = fabs(targetposition);
        double acc = (v - initVel) / accTime;
        double dec = (0 - v) / decTime;

        // double planDistance = fabs(0.5*acc*accTime*accTime + (double)initVel*accTime) + fabs(decTime*v*0.5);
        // if (planDistance > targetposition) return 1;

        double t2 = (targetposition - fabs(0.5 * acc * accTime * accTime + (double)initVel * accTime) - fabs(0.5 * v * v / dec)) / fabs(v) + accTime;
        double t3 = t2 + decTime;

        if (stepTime <= accTime)
        {
            *nowp = (int)(0.5 * acc * stepTime * stepTime + (double)initVel * stepTime);
            *nowvel = (int)(acc * stepTime + (double)initVel);
        }
        else if (stepTime <= t2)
        {
            *nowp = (int)(0.5 * acc * accTime * accTime + (double)initVel * accTime + v * (stepTime - accTime));
            *nowvel = (int)(v);
        }
        else if (stepTime <= t3)
        {
            *nowp = (int)(0.5 * acc * accTime * accTime + (double)(initVel)*accTime + v * (t2 - accTime) + 0.5 * dec * stepTime * stepTime - t3 * stepTime * dec - 0.5 * dec * t2 * t2 + t3 * t2 * dec);
            *nowvel = (int)(dec * stepTime - t3 * dec);
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if (abs(initVel) < 300)
            return 1;
        stepTime += circleTime;
        double t1 = stoptime * 0.001;
        double acc = -(double)(initVel) / t1;
        if (stepTime <= t1)
        {
            *nowp = (int)(0.5 * acc * stepTime * stepTime + (double)(initVel)*stepTime);
            *nowvel = acc * stepTime + (double)(initVel);
        }
        else
        {
            return 1;
        }
    }
    PRINTF("nowp=%d,nowvel=%d\r\n", nowp[0], nowvel[0]);
    return 0;
}

velplan_ move_J(int currentaxisdec[5], motorparam_ mp[5], double *spacepos, int vel)
{
    // MOVJ规划器
    // 当前各关节dec：int currentaxisdec[5]；
    // 各电机参数类型：motorparam_ mp[5]；
    // 目标坐标点位：spacepos[0]-[5];
    // 目标速度:vel（ms）
    velplan_ data;
    PRINTF("MOVJ currentaxisdec: [0]=%d,[1]=%d,[2]=%d,[3]=%d,[4]=%d\n",
           currentaxisdec[0], currentaxisdec[1], currentaxisdec[2], currentaxisdec[3], currentaxisdec[4]);
    PRINTF("MOVJ ENDpos: X=%f,Y=%f,Z=%f,oX=%f,oY=%f,oZ=%f,vel=%d\n",
           spacepos[0], spacepos[1], spacepos[2], spacepos[3], spacepos[4], spacepos[5], vel);
    for (int i = 0; i < 5; i++)
    {
        PRINTF("MOVJ motorparam_: direction=%d,initDec=%d,transmission=%f,dectime=%d,acctime=%d\n",
               mp[i].direction, mp[i].initDec, mp[i].transmission, mp[i].dectime, mp[i].acctime);
    }
    double targetp[5];
    int targetp_[5];
    int axisvel[5];
    if (vel < 700)
    {
        vel = 700;
    }
    //    printf("MOVJ ENDpos: X=%f,Y=%f,Z=%f,oX=%f,oY=%f,oZ=%f,vel=%d\n",
    //                     spacepos[0],spacepos[1],spacepos[2],spacepos[3],spacepos[4],spacepos[5],vel);
    sixToBaseEndlink(spacepos[0], spacepos[1], spacepos[2], spacepos[3], spacepos[4], spacepos[5], targetp);

    double joint[5];
    joint[0] = ((double)(currentaxisdec[0] * mp[0].direction - mp[0].initDec) /
                (mp[0].transmission * 10000.0));
    joint[1] = ((double)(currentaxisdec[1] * mp[1].direction - mp[1].initDec) /
                (mp[1].transmission * 10000.0));
    joint[2] = ((double)(currentaxisdec[2] * mp[2].direction - mp[2].initDec) /
                (mp[2].transmission * 10000.0));
    joint[3] = ((double)(currentaxisdec[3] * mp[3].direction - mp[3].initDec) /
                (mp[3].transmission * 10000.0));
    joint[4] = ((double)(currentaxisdec[4] * mp[4].direction - mp[4].initDec) /
                (mp[4].transmission * 10000.0));

    double posen[6];
    baseToSixEndlink(joint, posen);
    // printf("MOVJ STARTp: X=%f,Y=%f,Z=%f,oX[3]=%f,oY[4]=%f,oZ[5]=%f\n",posen[0],posen[1],posen[2],posen[3],posen[4],posen[5]);

    targetp_[0] = (int)((targetp[0] * (10000.0 * mp[0].transmission) + mp[0].initDec) * mp[0].direction);
    targetp_[1] = (int)((targetp[1] * (10000.0 * mp[1].transmission) + mp[1].initDec) * mp[1].direction);
    targetp_[2] = (int)((targetp[2] * (10000.0 * mp[2].transmission) + mp[2].initDec) * mp[2].direction);
    targetp_[3] = (int)((targetp[3] * (10000.0 * mp[3].transmission) + mp[3].initDec) * mp[3].direction);
    targetp_[4] = (int)((targetp[4] * (10000.0 * mp[4].transmission) + mp[4].initDec) * mp[4].direction);
    // printf("mvjendJoint jpos d1=%f,d2=%f,d3=%f,d4=%f,d5=%f\n",targetp_[0],targetp_[1],targetp_[2],targetp_[3],targetp_[4]);

    // for (int i = 0; i < 5; i++)
    //{
    //	printf("movj targetp_%d \n", targetp_[i]);
    // }
    // for (int i = 0; i < 5; i++)
    //{
    //	printf("movj nowpostion[i]%d \n", nowpostion[i]);
    // }

    for (int i = 0; i < 5; i++)
    {
        targetp_[i] = targetp_[i] - currentaxisdec[i];

        // if((abs(targetp_[3])-1000)<0)
        // targetp_[3] = 0;
        // if((abs(targetp_[4])-1000)<0)
        // targetp_[4] = 0;

        axisvel[i] = (int)((double)(targetp_[i]) / (double)(vel) * 1000.0);

        if (abs(targetp_[i]) < 30)
        {
            axisvel[i] = 0;
        }

        // printf("MOVJ axis:%d targetp_=%d vel=%d axisvel=%d\n", i+1,targetp_[i],vel,axisvel[i]);

        targetp_[i] = abs(targetp_[i]);
        data.acc[i] = (int)(mp[i].acctime);
        data.dec[i] = (int)(mp[i].dectime);
        data.velocity[i] = axisvel[i];
        data.targetposition[i] = targetp_[i];
        PRINTF("MOVJ axisnum:%d,acc=%d,dec=%d,axisvel=%d,targetposition=%d\n", i, data.acc[i], data.dec[i], data.velocity[i], data.targetposition[i]);
    }
    return data;
}

int filterMaxVelocity(double maxvelocity, int acctime, int dectime, int v, int targetdec, int *vel)
{
    // 速度限制器
    // double maxvelocity: 配置参数 最大速度

    // int acctime:插补参数
    // int dectime:插补参数
    // int v:插补参数
    // int targetdec:插补参数

    // int *vel:返回速度

    // return parameterOk;
    int parameterOk = 0;
    int velMax = v;
    targetdec = abs(targetdec);
    int planDistance = abs((double)(velMax - maxvelocity) / 2000.0 * (double)(acctime) + (double)(maxvelocity) / 1000.0 * (double)(acctime)) + abs((double)(velMax) / 2000.0 * (double)(dectime));

    if (planDistance > targetdec)
    {
        if (velMax < 0)
        {
            velMax = -((double)(targetdec) - (double)(maxvelocity) / 2000.0 * (double)(acctime)) / (((double)(acctime) + (double)(dectime)) / 2000.0);
        }
        else
        {
            velMax = ((double)(targetdec) - (double)(maxvelocity) / 2000.0 * (double)(acctime)) / (((double)(acctime) + (double)(dectime)) / 2000.0);
        }
        parameterOk = 1;
    }

    int accDistance = abs((double)(velMax - maxvelocity) / 2000.0 * (double)(acctime) + (double)(maxvelocity) / 1000.0 * (double)(acctime));

    if (accDistance > targetdec)
    {
        velMax = 0;
        parameterOk = 2;
    }

    *vel = velMax;

    return parameterOk;
}
