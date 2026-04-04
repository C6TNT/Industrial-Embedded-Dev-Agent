#include "libcrobot.h"

RobotAngle crobotdecToJoint(int currentaxisdec[6], motorparam_ mp[6])
{
    RobotAngle joint;
    // 功能：dec->关节坐标u
    joint.Angle1 = ((double)(currentaxisdec[0] * mp[0].direction - mp[0].initDec) /
                    (mp[0].transmission * 1000.0));
    joint.Angle2 = ((double)(currentaxisdec[1] * mp[1].direction - mp[1].initDec) /
                    (mp[1].transmission * 1000.0));
    joint.Angle3 = ((double)(currentaxisdec[2] * mp[2].direction - mp[2].initDec) /
                    (mp[2].transmission * 1000.0));
    joint.Angle4 = ((double)(currentaxisdec[3] * mp[3].direction - mp[3].initDec) /
                    (mp[3].transmission * 1000.0));
    joint.Angle5 = ((double)(currentaxisdec[4] * mp[4].direction - mp[4].initDec) /
                    (mp[4].transmission * 1000.0));
    joint.Angle6 = ((double)(currentaxisdec[5] * mp[5].direction - mp[5].initDec) /
                    (mp[5].transmission * 1000.0));
    joint.Angle1 = joint.Angle1 / PI * 180.0;
    joint.Angle2 = joint.Angle2 / PI * 180.0;
    joint.Angle3 = joint.Angle3 / PI * 180.0;
    joint.Angle4 = joint.Angle4 / PI * 180.0;
    joint.Angle5 = joint.Angle5 / PI * 180.0;
    joint.Angle6 = joint.Angle6 / PI * 180.0;
    return joint;
}

void crobotJointToDec(RobotAngle *joint, motorparam_ mp[6], int *calaxisdec)
{
    // 功能：关节坐标u->dec
    //    joint.Angle1= joint.Angle1*PI/180.0;
    //    joint.Angle2=joint.Angle2*PI/180.0;
    //    joint.Angle3=joint.Angle3*PI/180.0;
    //    joint.Angle4=joint.Angle4*PI/180.0;
    //    joint.Angle5=joint.Angle5*PI/180.0;
    //    joint.Angle6=joint.Angle6*PI/180.0;
    calaxisdec[0] = (int)((joint->Angle1 * PI / 180.0 * (mp[0].transmission * 1000.0) + mp[0].initDec) * mp[0].direction);
    calaxisdec[1] = (int)((joint->Angle2 * PI / 180.0 * (mp[1].transmission * 1000.0) + mp[1].initDec) * mp[1].direction);
    calaxisdec[2] = (int)((joint->Angle3 * PI / 180.0 * (mp[2].transmission * 1000.0) + mp[2].initDec) * mp[2].direction);
    calaxisdec[3] = (int)((joint->Angle4 * PI / 180.0 * (mp[3].transmission * 1000.0) + mp[3].initDec) * mp[3].direction);
    calaxisdec[4] = (int)((joint->Angle5 * PI / 180.0 * (mp[4].transmission * 1000.0) + mp[4].initDec) * mp[4].direction);
    calaxisdec[5] = (int)((joint->Angle6 * PI / 180.0 * (mp[5].transmission * 1000.0) + mp[5].initDec) * mp[5].direction);
}

void calculateInitdec(int currentdec[6], motorparam_ mp[6], double *initdec)
{
    initdec[0] = (currentdec[0] * mp[0].direction - 0.0 / 180.0 * PI * mp[0].transmission * 1000.0);
    initdec[1] = (currentdec[1] * mp[1].direction + 90.0 / 180.0 * PI * mp[1].transmission * 1000.0);
    initdec[2] = (currentdec[2] * mp[2].direction - 0.0 / 180.0 * PI * mp[2].transmission * 1000.0);
    initdec[3] = (currentdec[3] * mp[3].direction - 0.0 / 180.0 * PI * mp[3].transmission * 1000.0);
    initdec[4] = (currentdec[4] * mp[4].direction - 90.0 / 180.0 * PI * mp[4].transmission * 1000.0);
    initdec[5] = (currentdec[5] * mp[5].direction - 0.0 / 180.0 * PI * mp[5].transmission * 1000.0);
}

RobotPosition QuaternionToRotationMatrix(RobotQuationPos *quapostion)
{
    RobotPosition rerobotposition;
    //[0][1][2]
    //    [3]=.ox
    //    [4]=.oy
    //    [5]=.oz
    //    [6]=.ow
    // printf("@@@@@@%0.6f,%0.6f,%0.6f,%0.6f;\r\n", quapostion.ox, quapostion.oy, quapostion.ox, quapostion.ow);

    rerobotposition.nx = 1 - 2 * quapostion->oy * quapostion->oy - 2 * quapostion->oz * quapostion->oz;
    rerobotposition.ny = 2 * quapostion->ox * quapostion->oy + 2 * quapostion->oz * quapostion->ow;
    rerobotposition.nz = 2 * quapostion->ox * quapostion->oz - 2 * quapostion->oy * quapostion->ow;

    rerobotposition.ox = 2 * quapostion->ox * quapostion->oy - 2 * quapostion->oz * quapostion->ow;
    rerobotposition.oy = 1 - 2 * quapostion->ox * quapostion->ox - 2 * quapostion->oz * quapostion->oz;
    rerobotposition.oz = 2 * quapostion->oy * quapostion->oz + 2 * quapostion->ox * quapostion->ow;

    rerobotposition.ax = 2 * quapostion->ox * quapostion->oz + 2 * quapostion->oy * quapostion->ow;
    rerobotposition.ay = 2 * quapostion->oy * quapostion->oz - 2 * quapostion->ox * quapostion->ow;
    rerobotposition.az = 1 - 2 * quapostion->ox * quapostion->ox - 2 * quapostion->oy * quapostion->oy;

    rerobotposition.px = quapostion->x;
    rerobotposition.py = quapostion->y;
    rerobotposition.pz = quapostion->z;
    return rerobotposition;
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", rerobotposition.nx, rerobotposition.ox, rerobotposition.ax, rerobotposition.px);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", rerobotposition.ny, rerobotposition.oy, rerobotposition.ay, rerobotposition.py);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", rerobotposition.nz, rerobotposition.oz, rerobotposition.az, rerobotposition.pz);
}

RobotQuationPos RotationToQuaternionMatrix(RobotPosition *robotposition)
{
    RobotQuationPos quapostion;
    double trR = 0.0;
    double S = 0.0;
    trR = robotposition->nx + robotposition->oy + robotposition->az;
    if (trR > 0)
    {
        S = 2.0 * sqrt(trR + 1.0);
        quapostion.x = robotposition->px;
        quapostion.y = robotposition->py;
        quapostion.z = robotposition->pz;
        quapostion.ow = 0.25 * S;
        quapostion.ox = (robotposition->oz - robotposition->ay) / S;
        quapostion.oy = (robotposition->ax - robotposition->nz) / S;
        quapostion.oz = (robotposition->ny - robotposition->ox) / S;
        //        printf("quapostion.x=%3.10f\n", quapostion.x);
        //        printf("quapostion.y=%3.10f\n", quapostion.y);
        //        printf("quapostion.z=%3.10f\n", quapostion.z);
        //        printf("quapostion.ox=%3.10f\n", quapostion.ox);
        //        printf("quapostion.oy=%3.10f\n", quapostion.oy);
        //        printf("quapostion.oz=%3.10f\n", quapostion.oz);
        //        printf("quapostion.ow=%3.10f\n", quapostion.ow);
    }
    else if ((robotposition->nx > robotposition->oy) && (robotposition->nx > robotposition->az))
    {
        S = 2.0 * sqrt(1.0 + robotposition->nx - robotposition->oy - robotposition->az);
        quapostion.x = robotposition->px;
        quapostion.y = robotposition->py;
        quapostion.z = robotposition->pz;
        quapostion.ow = (robotposition->oz - robotposition->ay) / S;
        quapostion.ox = 0.25 * S;
        quapostion.oy = (robotposition->ox + robotposition->ny) / S;
        quapostion.oz = (robotposition->ax + robotposition->nz) / S;
        //        printf("quapostion.x=%3.10f\n", quapostion.x);
        //        printf("quapostion.y=%3.10f\n", quapostion.y);
        //        printf("quapostion.z=%3.10f\n", quapostion.z);
        //        printf("quapostion.ox=%3.10f\n", quapostion.ox);
        //        printf("quapostion.oy=%3.10f\n", quapostion.oy);
        //        printf("quapostion.oz=%3.10f\n", quapostion.oz);
        //        printf("quapostion.ow=%3.10f\n", quapostion.ow);
    }
    else if (robotposition->oy > robotposition->az)
    {
        S = 2.0 * sqrt(1.0 + robotposition->oy - robotposition->nx - robotposition->az);
        quapostion.x = robotposition->px;
        quapostion.y = robotposition->py;
        quapostion.z = robotposition->pz;
        quapostion.ow = (robotposition->ax - robotposition->nz) / S;
        quapostion.ox = (robotposition->ox + robotposition->ny) / S;
        quapostion.oy = 0.25 * S;
        quapostion.oz = (robotposition->ay + robotposition->oz) / S;
        //        printf("quapostion.x=%3.10f\n", quapostion.x);
        //        printf("quapostion.y=%3.10f\n", quapostion.y);
        //        printf("quapostion.z=%3.10f\n", quapostion.z);
        //        printf("quapostion.ox=%3.10f\n", quapostion.ox);
        //        printf("quapostion.oy=%3.10f\n", quapostion.oy);
        //        printf("quapostion.oz=%3.10f\n", quapostion.oz);
        //        printf("quapostion.ow=%3.10f\n", quapostion.ow);
    }
    else
    {
        S = 2.0 * sqrt(1.0 + robotposition->az - robotposition->nx - robotposition->oy);
        quapostion.x = robotposition->px;
        quapostion.y = robotposition->py;
        quapostion.z = robotposition->pz;
        quapostion.ow = (robotposition->ny - robotposition->ox) / S;
        quapostion.ox = (robotposition->ax + robotposition->nz) / S;
        quapostion.oy = (robotposition->ay + robotposition->oz) / S;
        quapostion.oz = 0.25 * S;
        //        printf("quapostion.x=%3.10f\n", quapostion.x);
        //        printf("quapostion.y=%3.10f\n", quapostion.y);
        //        printf("quapostion.z=%3.10f\n", quapostion.z);
        //        printf("quapostion.ox=%3.10f\n", quapostion.ox);
        //        printf("quapostion.oy=%3.10f\n", quapostion.oy);
        //        printf("quapostion.oz=%3.10f\n", quapostion.oz);
        //        printf("quapostion.ow=%3.10f\n", quapostion.ow);
    }
    return quapostion;
}
EulerAngles matrixToEulerAngles(RobotPosition tM_pos)
{
    EulerAngles angles = {0, 0, 0};
    double m00 = tM_pos.nx;
    double m01 = tM_pos.ox;
    double m02 = tM_pos.ax;
    double m10 = tM_pos.ny;
    double m11 = tM_pos.oy;
    double m12 = tM_pos.ay;
    double m20 = tM_pos.nz;
    // double m21 = tM_pos.oz;
    double m22 = tM_pos.az;

    // 1. 计算Pitch（弧度）
    double pitch_rad = asin(m02);
    double cos_pitch = cos(pitch_rad);

    if (fabs(cos_pitch) > 1e-6)
    {
        // 非万向锁：计算Roll和Yaw（弧度）
        double roll_rad = atan2(-m01, m00);
        double yaw_rad = atan2(-m12, m22);

        // 弧度转角度
        angles.roll = roll_rad * 180.0 / PI;
        angles.pitch = pitch_rad * 180.0 / PI;
        angles.yaw = yaw_rad * 180.0 / PI;
    }
    else
    {
        // 万向锁：Pitch为±90度，固定Roll=0
        if (pitch_rad > 0)
        {
            // Pitch ≈ 90度
            double yaw_roll_sum = atan2(m10, m11);
            angles.roll = 0.0;
            angles.pitch = 90.0;
            angles.yaw = yaw_roll_sum * 180.0 / PI;
        }
        else
        {
            // Pitch ≈ -90度
            double yaw_roll_diff = atan2(-m10, m11);
            angles.roll = 0.0;
            angles.pitch = -90.0;
            angles.yaw = yaw_roll_diff * 180.0 / PI;
        }
    }

    // 2. 调整Roll和Yaw到[-180, 180]范围
    if (angles.roll > 180.0)
        angles.roll -= 360.0;
    if (angles.roll < -180.0)
        angles.roll += 360.0;
    if (angles.yaw > 180.0)
        angles.yaw -= 360.0;
    if (angles.yaw < -180.0)
        angles.yaw += 360.0;

    angles.x = tM_pos.px;
    angles.y = tM_pos.py;
    angles.z = tM_pos.pz;
    return angles;
}
RobotPosition PositiveRobot(RobotAngle *robotangle)
{
    RobotPosition robotposition;
    // printf("IKFUN:%0.6f,%0.6f,%0.6f%0.6f,%0.6f,%0.6f;\r\n", robotangle.Angle1,robotangle.Angle2,robotangle.Angle3,
    //                                            robotangle.Angle4,robotangle.Angle5,robotangle.Angle6);
    RobotPosition temp;
    double s1 = robotangle->Angle1 * PI / 180.0;
    double s2 = robotangle->Angle2 * PI / 180.0;
    double s3 = robotangle->Angle3 * PI / 180.0;
    double s4 = robotangle->Angle4 * PI / 180.0;
    double s5 = robotangle->Angle5 * PI / 180.0;
    double s6 = robotangle->Angle6 * PI / 180.0;

    temp.nx = 1.0 * ((sin(s1) * sin(s4) + cos(s1) * cos(s4) * cos(s2 + s3)) * cos(s5) - sin(s5) * sin(s2 + s3) * cos(s1)) * cos(s6) + 1.0 * (sin(s1) * cos(s4) - 1.0 * sin(s4) * cos(s1) * cos(s2 + s3)) * sin(s6);
    temp.ny = 1.0 * ((sin(s1) * cos(s4) * cos(s2 + s3) - sin(s4) * cos(s1)) * cos(s5) - sin(s1) * sin(s5) * sin(s2 + s3)) * cos(s6) - 1.0 * (sin(s1) * sin(s4) * cos(s2 + s3) + cos(s1) * cos(s4)) * sin(s6);
    temp.nz = -1.0 * (sin(s5) * cos(s2 + s3) + sin(s2 + s3) * cos(s4) * cos(s5)) * cos(s6) + 1.0 * sin(s4) * sin(s6) * sin(s2 + s3);
    temp.ox = 1.0 * (-(sin(s1) * sin(s4) + cos(s1) * cos(s4) * cos(s2 + s3)) * cos(s5) + sin(s5) * sin(s2 + s3) * cos(s1)) * sin(s6) + 1.0 * (sin(s1) * cos(s4) - 1.0 * sin(s4) * cos(s1) * cos(s2 + s3)) * cos(s6);
    temp.oy = 1.0 * ((-sin(s1) * cos(s4) * cos(s2 + s3) + sin(s4) * cos(s1)) * cos(s5) + sin(s1) * sin(s5) * sin(s2 + s3)) * sin(s6) - 1.0 * (sin(s1) * sin(s4) * cos(s2 + s3) + cos(s1) * cos(s4)) * cos(s6);
    temp.oz = 1.0 * (sin(s5) * cos(s2 + s3) + sin(s2 + s3) * cos(s4) * cos(s5)) * sin(s6) + 1.0 * sin(s4) * sin(s2 + s3) * cos(s6);
    temp.ax = -1.0 * (sin(s1) * sin(s4) + cos(s1) * cos(s4) * cos(s2 + s3)) * sin(s5) - 1.0 * sin(s2 + s3) * cos(s1) * cos(s5);
    temp.ay = 1.0 * (-sin(s1) * cos(s4) * cos(s2 + s3) + sin(s4) * cos(s1)) * sin(s5) - 1.0 * sin(s1) * sin(s2 + s3) * cos(s5);
    temp.az = 1.0 * sin(s5) * sin(s2 + s3) * cos(s4) - 1.0 * cos(s5) * cos(s2 + s3);

    // temp.px = (-1.232 * sin(s2 + s3) + 1.111 * cos(s2) + 0.205 * cos(s2 + s3) + 0.32) * cos(s1);
    // temp.py = (-1.232 * sin(s2 + s3) + 1.111 * cos(s2) + 0.205 * cos(s2 + s3) + 0.32) * sin(s1);
    // temp.pz = -1.111 * sin(s2) - 0.205 * sin(s2 + s3) - 1.232 * cos(s2 + s3);
    temp.px = (-d4 * sin(s2 + s3) + a2 * cos(s2) + a3 * cos(s2 + s3) + a1) * cos(s1);
    temp.py = (-d4 * sin(s2 + s3) + a2 * cos(s2) + a3 * cos(s2 + s3) + a1) * sin(s1);
    temp.pz = -a2 * sin(s2) - a3 * sin(s2 + s3) - d4 * cos(s2 + s3);
    robotposition = temp;
    // printf("IKFUN:%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robotposition.nx, robotposition.ox, robotposition.ax, robotposition.px);
    // printf("IKFUN:%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robotposition.ny, robotposition.oy, robotposition.ay, robotposition.py);
    // printf("IKFUN:%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robotposition.nz, robotposition.oz, robotposition.az, robotposition.pz);
    return robotposition;
}

RobotAngle InverseRobot(const RobotPosition *robotposition, RobotAngle *lastrobotangle)
{
    RobotAngle robotangle;
    double tolrance = 0.3;
    double tan1, temp1, temp11, temp12;
    tan1 = robotposition->py / robotposition->px;
    if (fabs(tan1) < 1e-8)
    {
        if (fabs(PI * lastrobotangle->Angle1 / 180) < fabs(PI + PI * lastrobotangle->Angle1 / 180))
        {
            temp1 = 0;
        }
        else
        {
            temp1 = -PI;
        }
    }
    else
    {
        temp11 = atan(tan1);
        if (temp11 > 0)
        {
            temp12 = temp11 - PI;
        }
        else
        {
            temp12 = temp11 + PI;
        }
        if (fabs(temp11 - PI * lastrobotangle->Angle1 / 180) < fabs(temp12 - PI * lastrobotangle->Angle1 / 180))
        {
            temp1 = temp11;
        }
        else
        {
            temp1 = temp12;
        }
    }
    robotangle.Angle1 = temp1;
    double temp3a, temp3b, k;
    k = 0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(a2, 2) - pow(a3, 2) - pow(d4, 2)) / a2;

    temp3a = atan2(a3, d4) - atan2(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(a2, 2) - pow(a3, 2) - pow(d4, 2)) / a2, sqrt(a3 * a3 + d4 * d4 - pow(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(a2, 2) - pow(a3, 2) - pow(d4, 2)) / a2, 2)));
    temp3b = atan2(a3, d4) - atan2(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(a2, 2) - pow(a3, 2) - pow(d4, 2)) / a2, -sqrt(a3 * a3 + d4 * d4 - pow(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(a2, 2) - pow(a3, 2) - pow(d4, 2)) / a2, 2)));
    double k1 = -sqrt(a3 * a3 + d4 * d4 - pow(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(a2, 2) - pow(a3, 2) - pow(d4, 2)) / a2, 2));
    // printf("d4=%f,k1=%f\r\n", temp3a, k1);

    // printf("temp3a=%f,temp3b=%f\r\n", temp3a/PI*180.0, temp3b / PI * 180.0);
    double temp23, s23, c23;

    // 第一种解:
    robotangle.Angle3 = temp3a;
    // printf("temp3a\r\n");

    double p23;
    p23 = pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2);
    s23 = -(robotposition->pz * (a3 + a2 * cos(robotangle.Angle3)) + (cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1) * (d4 - a2 * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2));
    c23 = ((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1) * (a3 + a2 * cos(robotangle.Angle3)) - robotposition->pz * (d4 - a2 * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2));
    temp23 = atan2(s23, c23);
    // printf("temp23=%f\r\n", temp23/PI*180.0);
    if ((temp23 - robotangle.Angle3) > PI)
    {
        robotangle.Angle2 = 2 * PI - (temp23 - robotangle.Angle3);

        // printf("----temp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
    }
    else
    {
        robotangle.Angle2 = temp23 - robotangle.Angle3;
        // printf("----btemp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
    }

    double s4, c4;
    s4 = cos(robotangle.Angle1) * robotposition->ay - sin(robotangle.Angle1) * robotposition->ax;
    c4 = sin(robotangle.Angle2 + robotangle.Angle3) * robotposition->az - cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ax - sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ay;
    // printf("23=%f,s=%f,c=%f\r\n", robotangle.Angle3 /PI*180, s23, c23);
    robotangle.Angle4 = atan2(s4, c4);

    double s5, c5;
    s5 = robotposition->az * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->ax * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle1) * sin(robotangle.Angle4)) - robotposition->ay * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1));
    c5 = -robotposition->ax * (sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) - robotposition->ay * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->az * cos(robotangle.Angle2 + robotangle.Angle3);
    robotangle.Angle5 = atan2(s5, c5);
    if (abs(robotangle.Angle5) < 1e-5)
    {
        // robotangle.Angle4 = lastrobotangle.Angle4 / 180.0 * PI;
    }
    double s6, c6;
    c6 = robotposition->nx * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle4) * sin(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) +
         robotposition->ny * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3)) +
         robotposition->nz * (-cos(robotangle.Angle5) * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle5) * cos(robotangle.Angle2 + robotangle.Angle3));
    s6 = robotposition->nz * sin(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->nx * (sin(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - cos(robotangle.Angle4) * sin(robotangle.Angle1)) - robotposition->ny * (sin(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + cos(robotangle.Angle4) * cos(robotangle.Angle1));
    robotangle.Angle6 = atan2(s6, c6);
    // printf("%f,%f\r\n",s6,c6);
    // printf("-----------\r\n");
    // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle1 / PI * 180, robotangle.Angle2 / PI * 180, robotangle.Angle3 / PI * 180);
    // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle4 / PI * 180, robotangle.Angle5 / PI * 180, robotangle.Angle6 / PI * 180);
    if (isnan(robotangle.Angle1) || isnan(robotangle.Angle2) || isnan(robotangle.Angle3) ||
        isnan(robotangle.Angle4) || isnan(robotangle.Angle5) || isnan(robotangle.Angle6))
    {
        PRINTF("Error2 RESULT!\r\n");
        robotangle.Angle1 = lastrobotangle->Angle1;
        robotangle.Angle2 = lastrobotangle->Angle2;
        robotangle.Angle3 = lastrobotangle->Angle3;
        robotangle.Angle4 = lastrobotangle->Angle4;
        robotangle.Angle5 = lastrobotangle->Angle5;
        robotangle.Angle6 = lastrobotangle->Angle6;
        return robotangle;
    }
    if (abs(robotangle.Angle1 - lastrobotangle->Angle1 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle2 - lastrobotangle->Angle2 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle3 - lastrobotangle->Angle3 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle4 - lastrobotangle->Angle4 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle5 - lastrobotangle->Angle5 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle6 - lastrobotangle->Angle6 / 180.0 * PI) < tolrance)
    {
        // printf("FIRST RESULT!\r\n");
        robotangle.Angle1 = robotangle.Angle1 * 180.0 / PI;
        robotangle.Angle2 = robotangle.Angle2 * 180.0 / PI;
        robotangle.Angle3 = robotangle.Angle3 * 180.0 / PI;
        robotangle.Angle4 = robotangle.Angle4 * 180.0 / PI;
        robotangle.Angle5 = robotangle.Angle5 * 180.0 / PI;
        robotangle.Angle6 = robotangle.Angle6 * 180.0 / PI;
        return robotangle;
    }
    else
    {
        // 第二种解:
        robotangle.Angle3 = temp3b;
        // printf("temp3b\r\n");

        p23 = pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2);
        s23 = -(robotposition->pz * (a3 + a2 * cos(robotangle.Angle3)) + (cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1) * (d4 - a2 * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2));
        c23 = ((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1) * (a3 + a2 * cos(robotangle.Angle3)) - robotposition->pz * (d4 - a2 * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - a1), 2));
        temp23 = atan2(s23, c23);
        // printf("temp23=%f\r\n", temp23 / PI * 180.0);
        if ((temp23 - robotangle.Angle3) > PI)
        {
            robotangle.Angle2 = (temp23 - robotangle.Angle3) - 2 * PI;

            // printf("temp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
        }
        else
        {
            robotangle.Angle2 = temp23 - robotangle.Angle3;
            // printf("btemp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
        }

        s4 = cos(robotangle.Angle1) * robotposition->ay - sin(robotangle.Angle1) * robotposition->ax;
        c4 = sin(robotangle.Angle2 + robotangle.Angle3) * robotposition->az - cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ax - sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ay;
        // printf("23=%f,s4=%f,c4=%f\r\n", robotangle.Angle3 / PI * 180, s4, c4);
        robotangle.Angle4 = atan2(s4, c4);

        s5 = robotposition->az * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->ax * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle1) * sin(robotangle.Angle4)) - robotposition->ay * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1));
        c5 = -robotposition->ax * (sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) - robotposition->ay * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->az * cos(robotangle.Angle2 + robotangle.Angle3);
        robotangle.Angle5 = atan2(s5, c5);
        if (abs(robotangle.Angle5) < 1e-5)
        {
            // printf("Angle5=%f,Angle4=%f\r\n",robotangle.Angle5 / PI * 180.0,robotangle.Angle4 / PI * 180.0);
            // robotangle.Angle4 = lastrobotangle.Angle4 / 180.0 * PI;
        }
        c6 = robotposition->nx * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle4) * sin(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) +
             robotposition->ny * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3)) +
             robotposition->nz * (-cos(robotangle.Angle5) * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle5) * cos(robotangle.Angle2 + robotangle.Angle3));
        s6 = robotposition->nz * sin(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->nx * (sin(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - cos(robotangle.Angle4) * sin(robotangle.Angle1)) - robotposition->ny * (sin(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + cos(robotangle.Angle4) * cos(robotangle.Angle1));
        robotangle.Angle6 = atan2(s6, c6);
        // printf("%f,%f\r\n", s6, c6);
        // printf("-----------\r\n");
        // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle1 / PI * 180, robotangle.Angle2 / PI * 180, robotangle.Angle3 / PI * 180);
        // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle4 / PI * 180, robotangle.Angle5 / PI * 180, robotangle.Angle6 / PI * 180);
        if (abs(robotangle.Angle1 - lastrobotangle->Angle1 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle2 - lastrobotangle->Angle2 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle3 - lastrobotangle->Angle3 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle4 - lastrobotangle->Angle4 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle5 - lastrobotangle->Angle5 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle6 - lastrobotangle->Angle6 / 180.0 * PI) < tolrance)
        {
            // printf("SECOND RESULT\r\n");
            robotangle.Angle1 = robotangle.Angle1 * 180.0 / PI;
            robotangle.Angle2 = robotangle.Angle2 * 180.0 / PI;
            robotangle.Angle3 = robotangle.Angle3 * 180.0 / PI;
            robotangle.Angle4 = robotangle.Angle4 * 180.0 / PI;
            robotangle.Angle5 = robotangle.Angle5 * 180.0 / PI;
            robotangle.Angle6 = robotangle.Angle6 * 180.0 / PI;

            return robotangle;
        }
        else
        {
            robotangle.Angle1 = lastrobotangle->Angle1;
            robotangle.Angle2 = lastrobotangle->Angle2;
            robotangle.Angle3 = lastrobotangle->Angle3;
            robotangle.Angle4 = lastrobotangle->Angle4;
            robotangle.Angle5 = lastrobotangle->Angle5;
            robotangle.Angle6 = lastrobotangle->Angle6;
            PRINTF("INVERSE ERROR!\r\n");
        }
    }
    return robotangle;
}

int sinefittingPlan_(int step, double stoptime, int initVel_i, double circleTime, double tm, double td, double vMax, double targetposition, int *nowp, int *nowvel)
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

int targetdistanceCal(RobotQuationPos startpos, RobotQuationPos targetpos)
{
    // 检查输入的位置坐标是否为合理的值
    if (!isfinite(startpos.x) || !isfinite(startpos.y) || !isfinite(startpos.z) ||
        !isfinite(targetpos.x) || !isfinite(targetpos.y) || !isfinite(targetpos.z))
    {
        // 处理无效输入，这里简单返回 -1 表示错误
        return -1;
    }
    double targetdistance = 0.0;
    double temp = 0.0;
    temp = pow((startpos.x - targetpos.x), 2) + pow((startpos.y - targetpos.y), 2) + pow((startpos.z - targetpos.z), 2);
    targetdistance = sqrt(temp) * 1000.0;
    return (int)(targetdistance);
}
// 计算中间位置
RobotQuationPos nextQuationPostion(RobotQuationPos startpos, RobotQuationPos targetpos, float bili)
{
    // printf("startpos:%f,%f,%f,%f,%f,%f,%f\r\n",startpos.x,startpos.y,startpos.z,
    //       startpos.ox,startpos.oy,startpos.oz,startpos.ow);
    // printf("targetpos:%f,%f,%f,%f,%f,%f,%f\r\n",targetpos.x,targetpos.y,targetpos.z,
    //       targetpos.ox,targetpos.oy,targetpos.oz,targetpos.ow);
    // printf("rate:%f\r\n",bili);
    RobotQuationPos result;
    double startX = startpos.x;
    double startY = startpos.y;
    double startZ = startpos.z;
    double targetX = targetpos.x;
    double targetY = targetpos.y;
    double targetZ = targetpos.z;
    float starting[4];
    float ending[4];
    float quationresult[4];
    // 检查比例系数的有效性
    if (bili < 0.0 || bili > 1.0)
    {
        // 处理无效输入
        PRINTF("rate is error!\r\n");
    }

    // 计算中间位置的坐标
    result.x = startX + bili * (targetX - startX);
    result.y = startY + bili * (targetY - startY);
    result.z = startZ + bili * (targetZ - startZ);

    // 姿态改变
    starting[0] = startpos.ow;
    starting[1] = startpos.ox;
    starting[2] = startpos.oy;
    starting[3] = startpos.oz;

    ending[0] = targetpos.ow;
    ending[1] = targetpos.ox;
    ending[2] = targetpos.oy;
    ending[3] = targetpos.oz;
    // printf("zitaiqian?\r\n");
    slerpclaculate(starting, ending, quationresult, bili);
    // printf("zitaihou!\r\n");
    result.ox = quationresult[1];
    result.oy = quationresult[2];
    result.oz = quationresult[3];
    result.ow = quationresult[0];
    // printf("result:%f,%f,%f,%f,%f,%f,%f\r\n",result.x,result.y,result.z,
    //       result.ox,result.oy,result.oz,result.ow);
    return result;
}
// 四元数曲线插值
void slerpclaculate(float starting[4], float ending[4], float result[4], float t)
{
    float cosa = starting[0] * ending[0] + starting[1] * ending[1] + starting[2] * ending[2] + starting[3] * ending[3];

    // If the dot product is negative, the quaternions have opposite handed-ness and slerp won't take
    // the shorter path. Fix by reversing one quaternion.
    if (cosa < 0.0f)
    {
        ending[0] = -ending[0];
        ending[1] = -ending[1];
        ending[2] = -ending[2];
        ending[3] = -ending[3];
        cosa = -cosa;
    }

    float k0, k1;

    // If the inputs are too close for comfort, linearly interpolate
    if (cosa > 0.9995f)
    {
        k0 = 1.0f - t;
        k1 = t;
    }
    else
    {
        float sina = sqrt(1.0f - cosa * cosa);
        float a = atan2(sina, cosa);
        k0 = sin((1.0f - t) * a) / sina;
        k1 = sin(t * a) / sina;
    }
    result[0] = starting[0] * k0 + ending[0] * k1;
    result[1] = starting[1] * k0 + ending[1] * k1;
    result[2] = starting[2] * k0 + ending[2] * k1;
    result[3] = starting[3] * k0 + ending[3] * k1;
}
RobotQuationPos stopPlan(RobotQuationPos nowp, RobotQuationPos endp, double nowvel, double remainingdisance)
{
    //    printf("stopPlan nowp:%f,%f,%f\r\n",nowp.x,nowp.y,nowp.z);
    //    printf("stopPlan endp:%f,%f,%f\r\n",endp.x,endp.y,endp.z);
    //    printf("stopPlan nowvel:%f,remainingdisance:%f\r\n",nowvel,remainingdisance);
    double stoptime = 0.3;
    double deceleration = -nowvel / stoptime;
    double stopdistance = 0.5 * deceleration * stoptime * stoptime + nowvel * stoptime;
    //     printf("stopPlan nowvel:%f,remainingdisance:%f,stopdistance:%f\r\n",nowvel,remainingdisance,stopdistance);
    RobotQuationPos stopPos;
    stopPos.x = nowp.x + (endp.x - nowp.x) * (stopdistance * 1000.0 / remainingdisance);
    stopPos.y = nowp.y + (endp.y - nowp.y) * (stopdistance * 1000.0 / remainingdisance);
    stopPos.z = nowp.z + (endp.z - nowp.z) * (stopdistance * 1000.0 / remainingdisance);
    stopPos.ox = nowp.ox;
    stopPos.oy = nowp.oy;
    stopPos.oz = nowp.oz;
    stopPos.ow = nowp.ow;
    //    printf("stopPlan stopPos:%f,%f,%f\r\n",stopPos.x,stopPos.y,stopPos.z);
    return stopPos;
}

RobotPosition endLinkToSixConverter(RobotPosition endlinkpos, double tool[3])
{
    double al = 0.529;
    double bl = 0;
    double cl = 0.1505;
    al = tool[0];
    bl = tool[1];
    cl = tool[2];
    RobotPosition six_pos;
    six_pos.nx = endlinkpos.nx;
    six_pos.ny = endlinkpos.ny;
    six_pos.nz = endlinkpos.nz;

    six_pos.ox = endlinkpos.ox;
    six_pos.oy = endlinkpos.oy;
    six_pos.oz = endlinkpos.oz;

    six_pos.ax = endlinkpos.ax;
    six_pos.ay = endlinkpos.ay;
    six_pos.az = endlinkpos.az;

    six_pos.px = endlinkpos.nx * (0 - al) + endlinkpos.ox * (0 - bl) + endlinkpos.ax * (0 - cl) + endlinkpos.px;
    six_pos.py = endlinkpos.ny * (0 - al) + endlinkpos.oy * (0 - bl) + endlinkpos.ay * (0 - cl) + endlinkpos.py;
    six_pos.pz = endlinkpos.nz * (0 - al) + endlinkpos.oz * (0 - bl) + endlinkpos.az * (0 - cl) + endlinkpos.pz;
    return six_pos;
}
RobotPosition sixToendLinkConverter(RobotPosition six_pos, double tool[3])
{
    double al = 0.529;
    double bl = 0;
    double cl = 0.1505;
    al = tool[0];
    bl = tool[1];
    cl = tool[2];
    RobotPosition endlinkpos;
    endlinkpos.nx = six_pos.nx;
    endlinkpos.ny = six_pos.ny;
    endlinkpos.nz = six_pos.nz;

    endlinkpos.ox = six_pos.ox;
    endlinkpos.oy = six_pos.oy;
    endlinkpos.oz = six_pos.oz;

    endlinkpos.ax = six_pos.ax;
    endlinkpos.ay = six_pos.ay;
    endlinkpos.az = six_pos.az;

    endlinkpos.px = six_pos.nx * al + six_pos.ox * bl + six_pos.ax * cl + six_pos.px;
    endlinkpos.py = six_pos.ny * al + six_pos.oy * bl + six_pos.ay * cl + six_pos.py;
    endlinkpos.pz = six_pos.nz * al + six_pos.oz * bl + six_pos.az * cl + six_pos.pz;
    return endlinkpos;
}
RobotPosition calculate_vertical_sixbot(RobotPosition point_A, RobotPosition point_B, RobotPosition point_C, double d[3])
{
    // A右边第一个点 B向左一个点 C向下一个点
    RobotPosition result;
    double b0 = 0;
    double c0 = 0;

    b0 = point_B.px + d[1] - d[0];
    c0 = point_C.px + d[2] - d[0];

    double aB[3] = {0, 0, 0};
    double aC[3] = {0, 0, 0};

    aB[0] = b0 - point_A.px;
    aB[1] = point_B.py - point_A.py;
    aB[2] = point_B.pz - point_A.pz;
    aC[0] = c0 - point_A.px;
    aC[1] = point_C.py - point_A.py;
    aC[2] = point_C.pz - point_A.pz;

    double n[3] = {0, 0, 0};
    double o[3] = {0, 0, 0};
    double a[3] = {0, 0, 0};
    n[0] = aB[1] * aC[2] - aC[1] * aB[2];
    n[1] = aC[0] * aB[2] - aB[0] * aC[2];
    n[2] = aB[0] * aC[1] - aC[0] * aB[1];

    o[0] = 0 - aB[0];
    o[1] = 0 - aB[1];
    o[2] = 0 - aB[2];

    a[0] = n[1] * o[2] - o[1] * n[2];
    a[1] = o[0] * n[2] - n[0] * o[2];
    a[2] = n[0] * o[1] - o[0] * n[1];

    if (a[2] > 0)
    {
        a[0] = 0 - a[0];
        a[1] = 0 - a[1];
        a[2] = 0 - a[2];
        // printf("h1\n");
    }
    else if (a[2] == 0)
    {
        a[0] = 0;
        a[1] = 0;
        a[2] = -1;
        // printf("h2\n");
    }
    if (n[0] < 0)
    {
        n[0] = 0 - n[0];
        n[1] = 0 - n[1];
        n[2] = 0 - n[2];
        // printf("h3\n");
    }
    else if (n[0] == 0)
    {
        n[0] = 1;
        n[1] = 0;
        n[2] = 0;
        // printf("h4\n");
    }
    double mag1, mag2, mag3;
    mag1 = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    mag2 = sqrt(o[0] * o[0] + o[1] * o[1] + o[2] * o[2]);
    mag3 = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    PRINTF("%f  %f  %f\n", n[0] / mag1, o[0] / mag2, a[0] / mag3);
    PRINTF("%f  %f  %f\n", n[1] / mag1, o[1] / mag2, a[1] / mag3);
    PRINTF("%f  %f  %f\n", n[2] / mag1, o[2] / mag2, a[2] / mag3);
    result.nx = n[0] / mag1;
    result.ny = n[1] / mag1;
    result.nz = n[2] / mag1;

    result.ox = o[0] / mag2;
    result.oy = o[1] / mag2;
    result.oz = o[2] / mag2;

    result.ax = a[0] / mag3;
    result.ay = a[1] / mag3;
    result.az = a[2] / mag3;

    result.px = point_C.px;
    result.py = point_C.py;
    result.pz = point_C.pz;
    return result;
}

void cad_to_work1_trans_matrix(RobotPosition p_a0, double *cad_b0, RobotPosition p_a1, double *cad_b1, double *solution)
{
    // # p_a0 工件1：机器人坐标[x0, y0, z0]---对应---p_b0 cad坐标[x1, y1]
    // # p_a1 工件1：机器人坐标[x2, y2, z2]---对应---p_b1 cad坐标[x3, y3]

    double arr_L[4][4] = {0};
    double arr_R[4][4] = {0};
    arr_L[0][0] = 1;
    arr_L[0][1] = 0;
    arr_L[0][2] = cad_b0[0];
    arr_L[0][3] = -cad_b0[1];

    arr_L[1][0] = 0;
    arr_L[1][1] = 1;
    arr_L[1][2] = cad_b0[1];
    arr_L[1][3] = cad_b0[0];

    arr_L[2][0] = 1;
    arr_L[2][1] = 0;
    arr_L[2][2] = cad_b1[0];
    arr_L[2][3] = -cad_b1[1];

    arr_L[3][0] = 0;
    arr_L[3][1] = 1;
    arr_L[3][2] = cad_b1[1];
    arr_L[3][3] = cad_b1[0];

    matrix_inverse4(arr_L, arr_R);

    solution[0] = arr_R[0][0] * p_a0.pz + arr_R[0][1] * p_a0.py + arr_R[0][2] * p_a1.pz + arr_R[0][3] * p_a1.py;
    solution[1] = arr_R[1][0] * p_a0.pz + arr_R[1][1] * p_a0.py + arr_R[1][2] * p_a1.pz + arr_R[1][3] * p_a1.py;
    solution[2] = arr_R[2][0] * p_a0.pz + arr_R[2][1] * p_a0.py + arr_R[2][2] * p_a1.pz + arr_R[2][3] * p_a1.py;
    solution[3] = arr_R[3][0] * p_a0.pz + arr_R[3][1] * p_a0.py + arr_R[3][2] * p_a1.pz + arr_R[3][3] * p_a1.py;
    // # print('旋转矩阵')
    // # print(solution)
}

void cad_to_work1_coordinate(double *cad_pos, double *trans_matrix, double *deep, double *w1_pos)
{
    // # cad_pos cad坐标
    // # trans_matrix 变换矩阵
    // # print(' cad坐标' + str(cad_pos))
    w1_pos[0] = deep[0];
    w1_pos[1] = trans_matrix[3] * cad_pos[0] + trans_matrix[2] * cad_pos[1] + trans_matrix[1];
    w1_pos[2] = trans_matrix[2] * cad_pos[0] - trans_matrix[3] * cad_pos[1] + trans_matrix[0];
    // # w1_pos [x, y, z]工件2坐标
}

RobotPosition jog_x_released(RobotAngle initial_angle, double tool[3])
{
    // 实现x轴 - 释放函数
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    // printRobotAngle(j_rangle);
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    //::cout << "\n正向运动学结果:" << std::endl;
    // printRobotPosition(j_rposition);
    //  六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    // std::cout << "\n六转七结果1:" << std::endl;
    // printRobotPosition(j_endlink_pos);
    //  循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        j_endlink_pos.px -= 0.1; // x轴每次增加0.01m
        // 七转六
        j_six_pos = endLinkToSixConverter(j_endlink_pos, tool);
        // std::cout << "\n七转六结果:" << std::endl;
        // printRobotPosition(j_six_pos);
        // std::cout << "逆解传入上一次关节角度:" << std::endl;
        // printRobotAngle(j_tempangle);
        // std::cout << "逆解传入:" << std::endl;
        // printRobotPosition(j_six_pos);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // std::cout << "逆解关节角度:" << std::endl;
        // printRobotAngle(j_rerangle);
        //  检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // std::cout << "\n正向运动学结果2:" << std::endl;
            // printRobotPosition(j_rposition2);
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
            // std::cout << "\n六转七结果2:" << std::endl;
            // printRobotPosition(j_endlink_pos2);
        }
        else
        {
            // std::cout << "angle error\n"
            //           << std::endl;
            PRINTF("angle error\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((abs(j_endlink_pos2.py - j_endlink_pos.py) < 0.0001) &&
             (abs(j_endlink_pos2.px - j_endlink_pos.px) < 0.0001) &&
             (abs(j_endlink_pos2.pz - j_endlink_pos.pz) < 0.0001));

    j_endlink_pos.px += 0.1; // 回退最后一步（因为退出时已超范围）
    targetRobotPos = j_endlink_pos;
    return targetRobotPos;
}
// 实现x轴 + 释放函数
RobotPosition jog_x_forward(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    // std::cout << "初始关节角度:" << std::endl;
    // printRobotAngle(j_rangle);
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // std::cout << "\n正向运动学结果:" << std::endl;
    // printRobotPosition(j_rposition);
    //  六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    // std::cout << "\n六转七结果1:" << std::endl;
    // printRobotPosition(j_endlink_pos);
    //  循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        j_endlink_pos.px += 0.1; // x轴每次增加0.01m
        // 七转六
        j_six_pos = endLinkToSixConverter(j_endlink_pos, tool);
        // std::cout << "\n七转六结果:" << std::endl;
        // printRobotPosition(j_six_pos);
        // std::cout << "逆解传入上一次关节角度:" << std::endl;
        // printRobotAngle(j_tempangle);
        // std::cout << "逆解传入:" << std::endl;
        // printRobotPosition(j_six_pos);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // std::cout << "逆解关节角度:" << std::endl;
        // printRobotAngle(j_rerangle);
        //  检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // std::cout << "\n正向运动学结果2:" << std::endl;
            // printRobotPosition(j_rposition2);
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
            // std::cout << "\n六转七结果2:" << std::endl;
            // printRobotPosition(j_endlink_pos2);
        }
        else
        {
            // std::cout << "angle error\n"
            //           << std::endl;
            PRINTF("angle error\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((abs(j_endlink_pos2.py - j_endlink_pos.py) < 0.0001) &&
             (abs(j_endlink_pos2.px - j_endlink_pos.px) < 0.0001) &&
             (abs(j_endlink_pos2.pz - j_endlink_pos.pz) < 0.0001));

    j_endlink_pos.px -= 0.1; // 回退最后一步（因为退出时已超范围）
    targetRobotPos = j_endlink_pos;
    return targetRobotPos;
}

// 实现y轴 - 释放函数
RobotPosition jog_y_released(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    // std::cout << "初始关节角度:" << std::endl;
    // printRobotAngle(j_rangle);
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // std::cout << "\n正向运动学结果:" << std::endl;
    // printRobotPosition(j_rposition);
    //  六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    // std::cout << "\n六转七结果1:" << std::endl;
    // printRobotPosition(j_endlink_pos);
    //  循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        j_endlink_pos.py -= 0.1; // x轴每次增加0.01m
        // 七转六
        j_six_pos = endLinkToSixConverter(j_endlink_pos, tool);
        // std::cout << "\n七转六结果:" << std::endl;
        // printRobotPosition(j_six_pos);
        // std::cout << "逆解传入上一次关节角度:" << std::endl;
        // printRobotAngle(j_tempangle);
        // std::cout << "逆解传入:" << std::endl;
        // printRobotPosition(j_six_pos);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // std::cout << "逆解关节角度:" << std::endl;
        // printRobotAngle(j_rerangle);
        //  检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // std::cout << "\n正向运动学结果2:" << std::endl;
            // printRobotPosition(j_rposition2);
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
            // std::cout << "\n六转七结果2:" << std::endl;
            // printRobotPosition(j_endlink_pos2);
        }
        else
        {
            // std::cout << "angle error\n"
            //           << std::endl;
            PRINTF("angle error\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((abs(j_endlink_pos2.py - j_endlink_pos.py) < 0.0001) &&
             (abs(j_endlink_pos2.px - j_endlink_pos.px) < 0.0001) &&
             (abs(j_endlink_pos2.pz - j_endlink_pos.pz) < 0.0001));

    j_endlink_pos.py += 0.1; // 回退最后一步（因为退出时已超范围）
    targetRobotPos = j_endlink_pos;
    return targetRobotPos;
}
// 实现y轴 + 释放函数
RobotPosition jog_y_forward(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    // std::cout << "初始关节角度:" << std::endl;
    // printRobotAngle(j_rangle);
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // std::cout << "\n正向运动学结果:" << std::endl;
    // printRobotPosition(j_rposition);
    //  六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    // std::cout << "\n六转七结果1:" << std::endl;
    // printRobotPosition(j_endlink_pos);
    //  循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        j_endlink_pos.py += 0.1; // x轴每次增加0.01m
        // 七转六
        j_six_pos = endLinkToSixConverter(j_endlink_pos, tool);
        // std::cout << "\n七转六结果:" << std::endl;
        // printRobotPosition(j_six_pos);
        // std::cout << "逆解传入上一次关节角度:" << std::endl;
        // printRobotAngle(j_tempangle);
        // std::cout << "逆解传入:" << std::endl;
        // printRobotPosition(j_six_pos);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // std::cout << "逆解关节角度:" << std::endl;
        // printRobotAngle(j_rerangle);
        //  检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // std::cout << "\n正向运动学结果2:" << std::endl;
            // printRobotPosition(j_rposition2);
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
            // std::cout << "\n六转七结果2:" << std::endl;
            // printRobotPosition(j_endlink_pos2);
        }
        else
        {
            // std::cout << "angle error\n"
            //           << std::endl;
            PRINTF("angle error\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((abs(j_endlink_pos2.py - j_endlink_pos.py) < 0.0001) &&
             (abs(j_endlink_pos2.px - j_endlink_pos.px) < 0.0001) &&
             (abs(j_endlink_pos2.pz - j_endlink_pos.pz) < 0.0001));

    j_endlink_pos.py -= 0.1; // 回退最后一步（因为退出时已超范围）
    targetRobotPos = j_endlink_pos;
    return targetRobotPos;
}

// 实现z轴 - 释放函数
RobotPosition jog_z_released(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    // std::cout << "初始关节角度:" << std::endl;
    // printRobotAngle(j_rangle);
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // std::cout << "\n正向运动学结果:" << std::endl;
    // printRobotPosition(j_rposition);
    //  六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    // std::cout << "\n六转七结果1:" << std::endl;
    // printRobotPosition(j_endlink_pos);
    //  循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        j_endlink_pos.pz -= 0.1; // x轴每次增加0.01m
        // 七转六
        j_six_pos = endLinkToSixConverter(j_endlink_pos, tool);
        // std::cout << "\n七转六结果:" << std::endl;
        // printRobotPosition(j_six_pos);
        // std::cout << "逆解传入上一次关节角度:" << std::endl;
        // printRobotAngle(j_tempangle);
        // std::cout << "逆解传入:" << std::endl;
        // printRobotPosition(j_six_pos);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // std::cout << "逆解关节角度:" << std::endl;
        // printRobotAngle(j_rerangle);
        //  检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // std::cout << "\n正向运动学结果2:" << std::endl;
            // printRobotPosition(j_rposition2);
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
            // std::cout << "\n六转七结果2:" << std::endl;
            // printRobotPosition(j_endlink_pos2);
        }
        else
        {
            // std::cout << "angle error\n"
            //           << std::endl;
            PRINTF("angle error\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((abs(j_endlink_pos2.py - j_endlink_pos.py) < 0.0001) &&
             (abs(j_endlink_pos2.px - j_endlink_pos.px) < 0.0001) &&
             (abs(j_endlink_pos2.pz - j_endlink_pos.pz) < 0.0001));

    j_endlink_pos.pz += 0.1; // 回退最后一步（因为退出时已超范围）
    targetRobotPos = j_endlink_pos;
    return targetRobotPos;
}
// 实现z轴 + 释放函数
RobotPosition jog_z_forward(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    // std::cout << "初始关节角度:" << std::endl;
    // printRobotAngle(j_rangle);
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // std::cout << "\n正向运动学结果:" << std::endl;
    // printRobotPosition(j_rposition);
    //  六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    // std::cout << "\n六转七结果1:" << std::endl;
    // printRobotPosition(j_endlink_pos);
    //  循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        j_endlink_pos.pz += 0.1; // x轴每次增加0.01m
        // 七转六
        j_six_pos = endLinkToSixConverter(j_endlink_pos, tool);
        // std::cout << "\n七转六结果:" << std::endl;
        // printRobotPosition(j_six_pos);
        // std::cout << "逆解传入上一次关节角度:" << std::endl;
        // printRobotAngle(j_tempangle);
        // std::cout << "逆解传入:" << std::endl;
        // printRobotPosition(j_six_pos);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // std::cout << "逆解关节角度:" << std::endl;
        // printRobotAngle(j_rerangle);
        //  检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // std::cout << "\n正向运动学结果2:" << std::endl;
            // printRobotPosition(j_rposition2);
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
            // std::cout << "\n六转七结果2:" << std::endl;
            // printRobotPosition(j_endlink_pos2);
        }
        else
        {
            // std::cout << "angle error\n"
            //           << std::endl;
            PRINTF("angle error\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((abs(j_endlink_pos2.py - j_endlink_pos.py) < 0.0001) &&
             (abs(j_endlink_pos2.px - j_endlink_pos.px) < 0.0001) &&
             (abs(j_endlink_pos2.pz - j_endlink_pos.pz) < 0.0001));

    j_endlink_pos.pz -= 0.1; // 回退最后一步（因为退出时已超范围）
    targetRobotPos = j_endlink_pos;
    return targetRobotPos;
}
// 机器人转工件1
RobotPosition robot_trans_to_workpiece1(RobotPosition *origin_pos, RobotPosition *robot_space_pos)
{
    RobotPosition s_wp1;
    //    printf("原点\r\n");
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", origin_pos.nx, origin_pos.ox, origin_pos.ax, origin_pos.px);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", origin_pos.ny, origin_pos.oy, origin_pos.ay, origin_pos.py);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", origin_pos.nz, origin_pos.oz, origin_pos.az, origin_pos.pz);

    //    printf("机器人点位\r\n");
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robot_space_pos.nx, robot_space_pos.ox, robot_space_pos.ax, robot_space_pos.px);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robot_space_pos.ny, robot_space_pos.oy, robot_space_pos.ay, robot_space_pos.py);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robot_space_pos.nz, robot_space_pos.oz, robot_space_pos.az, robot_space_pos.pz);

    // s_wp2_origin[x ,y, z, R, P, Y]原点坐标
    // robot_space_pos---robot coordinates---num=6
    //*s_wp1---point int work1 coordinate system---num=6
    double s_wp2_T1[3][3] = {0};
    double s_wp2_T2[3][3] = {0};
    // p机器人原点坐标 xyz rpy
    // n
    s_wp2_T1[0][0] = origin_pos->nx;
    s_wp2_T1[1][0] = origin_pos->ny;
    s_wp2_T1[2][0] = origin_pos->nz;

    // o
    s_wp2_T1[0][1] = origin_pos->ox;
    s_wp2_T1[1][1] = origin_pos->oy;
    s_wp2_T1[2][1] = origin_pos->oz;

    // a
    s_wp2_T1[0][2] = origin_pos->ax;
    s_wp2_T1[1][2] = origin_pos->ay;
    s_wp2_T1[2][2] = origin_pos->az;
    //    printf("逆矩阵\r\n");
    //    printf("%0.6f,%0.6f,%0.6f;\r\n", s_wp2_T1[0][0], s_wp2_T1[0][1], s_wp2_T1[0][2]);
    //    printf("%0.6f,%0.6f,%0.6f;\r\n", s_wp2_T1[1][0], s_wp2_T1[1][1], s_wp2_T1[1][2]);
    //    printf("%0.6f,%0.6f,%0.6f;\r\n", s_wp2_T1[2][0], s_wp2_T1[2][1], s_wp2_T1[2][2]);
    // 求逆矩阵
    matrix_inverse(s_wp2_T1, s_wp2_T2);

    // 工件1 坐标
    s_wp1.px = s_wp2_T2[0][0] * (robot_space_pos->px - origin_pos->px) + s_wp2_T2[0][1] * (robot_space_pos->py - origin_pos->py) + s_wp2_T2[0][2] * (robot_space_pos->pz - origin_pos->pz);
    s_wp1.py = s_wp2_T2[1][0] * (robot_space_pos->px - origin_pos->px) + s_wp2_T2[1][1] * (robot_space_pos->py - origin_pos->py) + s_wp2_T2[1][2] * (robot_space_pos->pz - origin_pos->pz);
    s_wp1.pz = s_wp2_T2[2][0] * (robot_space_pos->px - origin_pos->px) + s_wp2_T2[2][1] * (robot_space_pos->py - origin_pos->py) + s_wp2_T2[2][2] * (robot_space_pos->pz - origin_pos->pz);

    s_wp1.nx = origin_pos->nx;
    s_wp1.ny = origin_pos->ny;
    s_wp1.nz = origin_pos->nz;

    s_wp1.ox = origin_pos->ox;
    s_wp1.oy = origin_pos->oy;
    s_wp1.oz = origin_pos->oz;

    s_wp1.ax = origin_pos->ax;
    s_wp1.ay = origin_pos->ay;
    s_wp1.az = origin_pos->az;
    //    printf("工件1\r\n");
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", s_wp1.nx, s_wp1.ox, s_wp1.ax, s_wp1.px);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", s_wp1.ny, s_wp1.oy, s_wp1.ay, s_wp1.py);
    //    printf("%0.6f,%0.6f,%0.6f,%0.6f;\r\n", s_wp1.nz, s_wp1.oz, s_wp1.az, s_wp1.pz);
    return s_wp1;
}

RobotPosition workpiece1_trans_to_robot(RobotPosition workpiece_1_origin, RobotPosition workpiece_1_position)
{
    // workpiece_1_origin 工件1的原点在机器人坐标系里的坐标---num=6
    // workpiece_1_position 工件1的坐标值---num=3
    // w_trc_out---robot coordinates---num=6
    RobotPosition w_trc_out;
    double w_trc_T0[3] = {0, 0, 0};
    double w_trc_T1[3] = {0, 0, 0};
    double w_trc_T2[3] = {0, 0, 0};

    w_trc_T0[0] = workpiece_1_origin.nx;
    w_trc_T1[0] = workpiece_1_origin.ny;
    w_trc_T2[0] = workpiece_1_origin.nz;

    w_trc_T0[1] = workpiece_1_origin.ox;
    w_trc_T1[1] = workpiece_1_origin.oy;
    w_trc_T2[1] = workpiece_1_origin.oz;

    w_trc_T0[2] = workpiece_1_origin.ax;
    w_trc_T1[2] = workpiece_1_origin.ay;
    w_trc_T2[2] = workpiece_1_origin.az;
    w_trc_out.px = w_trc_T0[0] * workpiece_1_position.px + w_trc_T0[1] * workpiece_1_position.py + w_trc_T0[2] * workpiece_1_position.pz + workpiece_1_origin.px;
    w_trc_out.py = w_trc_T1[0] * workpiece_1_position.px + w_trc_T1[1] * workpiece_1_position.py + w_trc_T1[2] * workpiece_1_position.pz + workpiece_1_origin.py;
    w_trc_out.pz = w_trc_T2[0] * workpiece_1_position.px + w_trc_T2[1] * workpiece_1_position.py + w_trc_T2[2] * workpiece_1_position.pz + workpiece_1_origin.pz;

    w_trc_out.nx = workpiece_1_origin.nx;
    w_trc_out.ny = workpiece_1_origin.ny;
    w_trc_out.nz = workpiece_1_origin.nz;

    w_trc_out.ox = workpiece_1_origin.ox;
    w_trc_out.oy = workpiece_1_origin.oy;
    w_trc_out.oz = workpiece_1_origin.oz;

    w_trc_out.ax = workpiece_1_origin.ax;
    w_trc_out.ay = workpiece_1_origin.ay;
    w_trc_out.az = workpiece_1_origin.az;
    return w_trc_out;
}

RobotPosition workpiece1_jog_x_released(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos, j_temp_endlink_pos;
    RobotPosition sw_p1;
    RobotPosition w_trc_out;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // 六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    j_temp_endlink_pos = j_endlink_pos;
    // 七转工件1
    sw_p1 = robot_trans_to_workpiece1(&j_temp_endlink_pos, &j_endlink_pos);
    // 循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        sw_p1.px -= 0.1;
        // 工件1转七
        w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
        // 七转六
        j_six_pos = endLinkToSixConverter(w_trc_out, tool);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // 检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0 &&
            fabs(j_rerangle.Angle6) < 180)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // 第二次六转七
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
        }
        else
        {
            // std::cout << "\n关节角度超出安全范围，退出循环" << std::endl;
            PRINTF("ANGLE LOOP FAILED\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((fabs(j_endlink_pos2.py - w_trc_out.py) < 0.0001) &&
             (fabs(j_endlink_pos2.px - w_trc_out.px) < 0.0001) &&
             (fabs(j_endlink_pos2.pz - w_trc_out.pz) < 0.0001));

    sw_p1.px += 0.1; // 回退最后一步（因为退出时已超范围）
    w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
    targetRobotPos = w_trc_out;
    return targetRobotPos;
}
RobotPosition workpiece1_jog_y_released(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos, j_temp_endlink_pos;
    RobotPosition sw_p1;
    RobotPosition w_trc_out;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // 六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    j_temp_endlink_pos = j_endlink_pos;
    // 七转工件1
    sw_p1 = robot_trans_to_workpiece1(&j_temp_endlink_pos, &j_endlink_pos);
    // 循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        sw_p1.py -= 0.1;
        // 工件1转七
        w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
        // 七转六
        j_six_pos = endLinkToSixConverter(w_trc_out, tool);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // 检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0 &&
            fabs(j_rerangle.Angle6) < 180)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // 第二次六转七
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
        }
        else
        {
            // std::cout << "\n关节角度超出安全范围，退出循环" << std::endl;
            PRINTF("ANGLE LOOP FAILED\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((fabs(j_endlink_pos2.py - w_trc_out.py) < 0.0001) &&
             (fabs(j_endlink_pos2.px - w_trc_out.px) < 0.0001) &&
             (fabs(j_endlink_pos2.pz - w_trc_out.pz) < 0.0001));

    sw_p1.py += 0.1; // 回退最后一步（因为退出时已超范围）
    w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
    targetRobotPos = w_trc_out;
    return targetRobotPos;
}
RobotPosition workpiece1_jog_z_released(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos, j_temp_endlink_pos;
    RobotPosition sw_p1;
    RobotPosition w_trc_out;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // 六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    j_temp_endlink_pos = j_endlink_pos;
    // 七转工件1
    sw_p1 = robot_trans_to_workpiece1(&j_temp_endlink_pos, &j_endlink_pos);
    // 循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        sw_p1.pz -= 0.1;
        // 工件1转七
        w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
        // 七转六
        j_six_pos = endLinkToSixConverter(w_trc_out, tool);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // 检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0 &&
            fabs(j_rerangle.Angle6) < 180)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // 第二次六转七
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
        }
        else
        {
            // std::cout << "\n关节角度超出安全范围，退出循环" << std::endl;
            PRINTF("ANGLE LOOP FAILED\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((fabs(j_endlink_pos2.py - w_trc_out.py) < 0.0001) &&
             (fabs(j_endlink_pos2.px - w_trc_out.px) < 0.0001) &&
             (fabs(j_endlink_pos2.pz - w_trc_out.pz) < 0.0001));

    sw_p1.pz += 0.1; // 回退最后一步（因为退出时已超范围）
    w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
    targetRobotPos = w_trc_out;
    return targetRobotPos;
}

RobotPosition workpiece1_jog_x_forward(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos, j_temp_endlink_pos;
    RobotPosition sw_p1;
    RobotPosition w_trc_out;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // 六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    j_temp_endlink_pos = j_endlink_pos;
    // 七转工件1
    sw_p1 = robot_trans_to_workpiece1(&j_temp_endlink_pos, &j_endlink_pos);
    // 循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        sw_p1.px += 0.1;
        // 工件1转七
        w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
        // 七转六
        j_six_pos = endLinkToSixConverter(w_trc_out, tool);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // 检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0 &&
            fabs(j_rerangle.Angle6) < 180)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // 第二次六转七
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
        }
        else
        {
            // std::cout << "\n关节角度超出安全范围，退出循环" << std::endl;
            PRINTF("ANGLE LOOP FAILED\r\n");
            break; // 角度超范围，退出循环
        }
    } while ((fabs(j_endlink_pos2.py - w_trc_out.py) < 0.0001) &&
             (fabs(j_endlink_pos2.px - w_trc_out.px) < 0.0001) &&
             (fabs(j_endlink_pos2.pz - w_trc_out.pz) < 0.0001));

    sw_p1.px -= 0.1; // 回退最后一步（因为退出时已超范围）
    w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
    targetRobotPos = w_trc_out;
    return targetRobotPos;
}
RobotPosition workpiece1_jog_y_forward(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos, j_temp_endlink_pos;
    RobotPosition sw_p1;
    RobotPosition w_trc_out;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // 六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    j_temp_endlink_pos = j_endlink_pos;
    // 七转工件1
    sw_p1 = robot_trans_to_workpiece1(&j_temp_endlink_pos, &j_endlink_pos);
    // 循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        sw_p1.py += 0.1;
        // 工件1转七
        w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
        // 七转六
        j_six_pos = endLinkToSixConverter(w_trc_out, tool);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // 检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0 &&
            fabs(j_rerangle.Angle6) < 180)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // 第二次六转七
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
        }
        else
        {
            PRINTF("ANGLE LOOP FAILED\r\n");
            // std::cout << "\n关节角度超出安全范围，退出循环" << std::endl;
            break; // 角度超范围，退出循环
        }
    } while ((fabs(j_endlink_pos2.py - w_trc_out.py) < 0.0001) &&
             (fabs(j_endlink_pos2.px - w_trc_out.px) < 0.0001) &&
             (fabs(j_endlink_pos2.pz - w_trc_out.pz) < 0.0001));

    sw_p1.py -= 0.1; // 回退最后一步（因为退出时已超范围）
    w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
    targetRobotPos = w_trc_out;
    return targetRobotPos;
}
RobotPosition workpiece1_jog_z_forward(RobotAngle initial_angle, double tool[3])
{
    RobotAngle j_rangle = initial_angle, j_rerangle;
    RobotPosition j_rposition, j_rposition2, j_endlink_pos, j_six_pos, j_endlink_pos2;
    RobotAngle j_tempangle;
    RobotPosition targetRobotPos, j_temp_endlink_pos;
    RobotPosition sw_p1;
    RobotPosition w_trc_out;
    // double tool[3] = { 0.1, 0.0, 0.2 };  // 工具参数（x,y,z偏移）
    //  正解
    j_rposition = PositiveRobot(&j_rangle);
    // 六转七：
    j_endlink_pos = sixToendLinkConverter(j_rposition, tool);
    j_temp_endlink_pos = j_endlink_pos;
    // 七转工件1
    sw_p1 = robot_trans_to_workpiece1(&j_temp_endlink_pos, &j_endlink_pos);
    // 循环调整x轴位置，直到关节角度超出范围或位置收敛
    j_tempangle = j_rangle;
    do
    {
        sw_p1.pz += 0.1;
        // 工件1转七
        w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
        // 七转六
        j_six_pos = endLinkToSixConverter(w_trc_out, tool);
        j_rerangle = InverseRobot(&j_six_pos, &j_tempangle); // 逆向求解新角度
        j_tempangle = j_rerangle;
        // 检查关节角度是否在安全范围
        if (fabs(j_rerangle.Angle1) < 60.0 &&
            j_rerangle.Angle2 < -40.0 && j_rerangle.Angle2 > -120.0 &&
            j_rerangle.Angle3 < 60.0 && j_rerangle.Angle3 > -45.0 &&
            fabs(j_rerangle.Angle4) < 45.0 &&
            j_rerangle.Angle5 < 120.0 && j_rerangle.Angle5 > 35.0 &&
            fabs(j_rerangle.Angle6) < 180)
        {
            // 第二次正解
            j_rposition2 = PositiveRobot(&j_rerangle);
            // 第二次六转七
            j_endlink_pos2 = sixToendLinkConverter(j_rposition2, tool);
        }
        else
        {
            PRINTF("ANGLE LOOP FAILED\r\n");
            // std::cout << "\n关节角度超出安全范围，退出循环" << std::endl;
            break; // 角度超范围，退出循环
        }
    } while ((fabs(j_endlink_pos2.py - w_trc_out.py) < 0.0001) &&
             (fabs(j_endlink_pos2.px - w_trc_out.px) < 0.0001) &&
             (fabs(j_endlink_pos2.pz - w_trc_out.pz) < 0.0001));

    sw_p1.pz -= 0.1; // 回退最后一步（因为退出时已超范围）
    w_trc_out = workpiece1_trans_to_robot(j_temp_endlink_pos, sw_p1);
    targetRobotPos = w_trc_out;
    return targetRobotPos;
}

RobotPosition PositiveRobot_dh(RobotAngle *robotangle, double dhparam[4])
{
    // dhparam[0] = a1
    // dhparam[1] = a2
    // dhparam[2] = a3
    // dhparam[3] = d4
    RobotPosition robotposition;
    // printf("IKFUN:%0.6f,%0.6f,%0.6f%0.6f,%0.6f,%0.6f;\r\n", robotangle.Angle1,robotangle.Angle2,robotangle.Angle3,
    //                                            robotangle.Angle4,robotangle.Angle5,robotangle.Angle6);
    RobotPosition temp;
    double s1 = robotangle->Angle1 * PI / 180.0;
    double s2 = robotangle->Angle2 * PI / 180.0;
    double s3 = robotangle->Angle3 * PI / 180.0;
    double s4 = robotangle->Angle4 * PI / 180.0;
    double s5 = robotangle->Angle5 * PI / 180.0;
    double s6 = robotangle->Angle6 * PI / 180.0;

    temp.nx = 1.0 * ((sin(s1) * sin(s4) + cos(s1) * cos(s4) * cos(s2 + s3)) * cos(s5) - sin(s5) * sin(s2 + s3) * cos(s1)) * cos(s6) + 1.0 * (sin(s1) * cos(s4) - 1.0 * sin(s4) * cos(s1) * cos(s2 + s3)) * sin(s6);
    temp.ny = 1.0 * ((sin(s1) * cos(s4) * cos(s2 + s3) - sin(s4) * cos(s1)) * cos(s5) - sin(s1) * sin(s5) * sin(s2 + s3)) * cos(s6) - 1.0 * (sin(s1) * sin(s4) * cos(s2 + s3) + cos(s1) * cos(s4)) * sin(s6);
    temp.nz = -1.0 * (sin(s5) * cos(s2 + s3) + sin(s2 + s3) * cos(s4) * cos(s5)) * cos(s6) + 1.0 * sin(s4) * sin(s6) * sin(s2 + s3);
    temp.ox = 1.0 * (-(sin(s1) * sin(s4) + cos(s1) * cos(s4) * cos(s2 + s3)) * cos(s5) + sin(s5) * sin(s2 + s3) * cos(s1)) * sin(s6) + 1.0 * (sin(s1) * cos(s4) - 1.0 * sin(s4) * cos(s1) * cos(s2 + s3)) * cos(s6);
    temp.oy = 1.0 * ((-sin(s1) * cos(s4) * cos(s2 + s3) + sin(s4) * cos(s1)) * cos(s5) + sin(s1) * sin(s5) * sin(s2 + s3)) * sin(s6) - 1.0 * (sin(s1) * sin(s4) * cos(s2 + s3) + cos(s1) * cos(s4)) * cos(s6);
    temp.oz = 1.0 * (sin(s5) * cos(s2 + s3) + sin(s2 + s3) * cos(s4) * cos(s5)) * sin(s6) + 1.0 * sin(s4) * sin(s2 + s3) * cos(s6);
    temp.ax = -1.0 * (sin(s1) * sin(s4) + cos(s1) * cos(s4) * cos(s2 + s3)) * sin(s5) - 1.0 * sin(s2 + s3) * cos(s1) * cos(s5);
    temp.ay = 1.0 * (-sin(s1) * cos(s4) * cos(s2 + s3) + sin(s4) * cos(s1)) * sin(s5) - 1.0 * sin(s1) * sin(s2 + s3) * cos(s5);
    temp.az = 1.0 * sin(s5) * sin(s2 + s3) * cos(s4) - 1.0 * cos(s5) * cos(s2 + s3);

    // temp.px = (-1.232 * sin(s2 + s3) + 1.111 * cos(s2) + 0.205 * cos(s2 + s3) + 0.32) * cos(s1);
    // temp.py = (-1.232 * sin(s2 + s3) + 1.111 * cos(s2) + 0.205 * cos(s2 + s3) + 0.32) * sin(s1);
    // temp.pz = -1.111 * sin(s2) - 0.205 * sin(s2 + s3) - 1.232 * cos(s2 + s3);
    temp.px = (-dhparam[3] * sin(s2 + s3) + dhparam[1] * cos(s2) + dhparam[2] * cos(s2 + s3) + dhparam[0]) * cos(s1);
    temp.py = (-dhparam[3] * sin(s2 + s3) + dhparam[1] * cos(s2) + dhparam[2] * cos(s2 + s3) + dhparam[0]) * sin(s1);
    temp.pz = -dhparam[1] * sin(s2) - dhparam[2] * sin(s2 + s3) - dhparam[3] * cos(s2 + s3);
    robotposition = temp;
    // printf("IKFUN:%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robotposition.nx, robotposition.ox, robotposition.ax, robotposition.px);
    // printf("IKFUN:%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robotposition.ny, robotposition.oy, robotposition.ay, robotposition.py);
    // printf("IKFUN:%0.6f,%0.6f,%0.6f,%0.6f;\r\n", robotposition.nz, robotposition.oz, robotposition.az, robotposition.pz);
    return robotposition;
}

RobotAngle InverseRobot_dh(const RobotPosition *robotposition, RobotAngle *lastrobotangle, double dhparam[4])
{
    RobotAngle robotangle;
    double tolrance = 0.3;
    double tan1, temp1, temp11, temp12;
    tan1 = robotposition->py / robotposition->px;
    if (fabs(tan1) < 1e-8)
    {
        if (fabs(PI * lastrobotangle->Angle1 / 180) < fabs(PI + PI * lastrobotangle->Angle1 / 180))
        {
            temp1 = 0;
        }
        else
        {
            temp1 = -PI;
        }
    }
    else
    {
        temp11 = atan(tan1);
        if (temp11 > 0)
        {
            temp12 = temp11 - PI;
        }
        else
        {
            temp12 = temp11 + PI;
        }
        if (fabs(temp11 - PI * lastrobotangle->Angle1 / 180) < fabs(temp12 - PI * lastrobotangle->Angle1 / 180))
        {
            temp1 = temp11;
        }
        else
        {
            temp1 = temp12;
        }
    }
    robotangle.Angle1 = temp1;
    double temp3a, temp3b, k;
    k = 0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(dhparam[1], 2) - pow(dhparam[2], 2) - pow(dhparam[3], 2)) / dhparam[1];

    temp3a = atan2(dhparam[2], dhparam[3]) - atan2(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(dhparam[1], 2) - pow(dhparam[2], 2) - pow(dhparam[3], 2)) / dhparam[1], sqrt(dhparam[2] * dhparam[2] + dhparam[3] * dhparam[3] - pow(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(dhparam[1], 2) - pow(dhparam[2], 2) - pow(dhparam[3], 2)) / dhparam[1], 2)));
    temp3b = atan2(dhparam[2], dhparam[3]) - atan2(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(dhparam[1], 2) - pow(dhparam[2], 2) - pow(dhparam[3], 2)) / dhparam[1], -sqrt(dhparam[2] * dhparam[2] + dhparam[3] * dhparam[3] - pow(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(dhparam[1], 2) - pow(dhparam[2], 2) - pow(dhparam[3], 2)) / dhparam[1], 2)));
    double k1 = -sqrt(dhparam[2] * dhparam[2] + dhparam[3] * dhparam[3] - pow(0.5 * (pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2) + pow(robotposition->pz, 2) + pow((-sin(robotangle.Angle1) * robotposition->px + cos(robotangle.Angle1) * robotposition->py), 2) - pow(dhparam[1], 2) - pow(dhparam[2], 2) - pow(dhparam[3], 2)) / dhparam[1], 2));
    // printf("dhparam[3]=%f,k1=%f\r\n", temp3a, k1);

    // printf("temp3a=%f,temp3b=%f\r\n", temp3a/PI*180.0, temp3b / PI * 180.0);
    double temp23, s23, c23;

    // 第一种解:
    robotangle.Angle3 = temp3a;
    // printf("temp3a\r\n");

    double p23;
    p23 = pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2);
    s23 = -(robotposition->pz * (dhparam[2] + dhparam[1] * cos(robotangle.Angle3)) + (cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]) * (dhparam[3] - dhparam[1] * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2));
    c23 = ((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]) * (dhparam[2] + dhparam[1] * cos(robotangle.Angle3)) - robotposition->pz * (dhparam[3] - dhparam[1] * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2));
    temp23 = atan2(s23, c23);
    // printf("temp23=%f\r\n", temp23/PI*180.0);
    if ((temp23 - robotangle.Angle3) > PI)
    {
        robotangle.Angle2 = 2 * PI - (temp23 - robotangle.Angle3);

        // printf("----temp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
    }
    else
    {
        robotangle.Angle2 = temp23 - robotangle.Angle3;
        // printf("----btemp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
    }

    double s4, c4;
    s4 = cos(robotangle.Angle1) * robotposition->ay - sin(robotangle.Angle1) * robotposition->ax;
    c4 = sin(robotangle.Angle2 + robotangle.Angle3) * robotposition->az - cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ax - sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ay;
    // printf("23=%f,s=%f,c=%f\r\n", robotangle.Angle3 /PI*180, s23, c23);
    robotangle.Angle4 = atan2(s4, c4);

    double s5, c5;
    s5 = robotposition->az * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->ax * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle1) * sin(robotangle.Angle4)) - robotposition->ay * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1));
    c5 = -robotposition->ax * (sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) - robotposition->ay * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->az * cos(robotangle.Angle2 + robotangle.Angle3);
    robotangle.Angle5 = atan2(s5, c5);
    if (abs(robotangle.Angle5) < 1e-5)
    {
        // robotangle.Angle4 = lastrobotangle.Angle4 / 180.0 * PI;
    }
    double s6, c6;
    c6 = robotposition->nx * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle4) * sin(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) +
         robotposition->ny * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3)) +
         robotposition->nz * (-cos(robotangle.Angle5) * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle5) * cos(robotangle.Angle2 + robotangle.Angle3));
    s6 = robotposition->nz * sin(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->nx * (sin(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - cos(robotangle.Angle4) * sin(robotangle.Angle1)) - robotposition->ny * (sin(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + cos(robotangle.Angle4) * cos(robotangle.Angle1));
    robotangle.Angle6 = atan2(s6, c6);
    // printf("%f,%f\r\n",s6,c6);
    // printf("-----------\r\n");
    // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle1 / PI * 180, robotangle.Angle2 / PI * 180, robotangle.Angle3 / PI * 180);
    // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle4 / PI * 180, robotangle.Angle5 / PI * 180, robotangle.Angle6 / PI * 180);
    if (isnan(robotangle.Angle1) || isnan(robotangle.Angle2) || isnan(robotangle.Angle3) ||
        isnan(robotangle.Angle4) || isnan(robotangle.Angle5) || isnan(robotangle.Angle6))
    {
        PRINTF("Error2 RESULT!\r\n");
        robotangle.Angle1 = lastrobotangle->Angle1;
        robotangle.Angle2 = lastrobotangle->Angle2;
        robotangle.Angle3 = lastrobotangle->Angle3;
        robotangle.Angle4 = lastrobotangle->Angle4;
        robotangle.Angle5 = lastrobotangle->Angle5;
        robotangle.Angle6 = lastrobotangle->Angle6;
        return robotangle;
    }
    if (abs(robotangle.Angle1 - lastrobotangle->Angle1 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle2 - lastrobotangle->Angle2 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle3 - lastrobotangle->Angle3 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle4 - lastrobotangle->Angle4 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle5 - lastrobotangle->Angle5 / 180.0 * PI) < tolrance &&
        abs(robotangle.Angle6 - lastrobotangle->Angle6 / 180.0 * PI) < tolrance)
    {
        // printf("FIRST RESULT!\r\n");
        robotangle.Angle1 = robotangle.Angle1 * 180.0 / PI;
        robotangle.Angle2 = robotangle.Angle2 * 180.0 / PI;
        robotangle.Angle3 = robotangle.Angle3 * 180.0 / PI;
        robotangle.Angle4 = robotangle.Angle4 * 180.0 / PI;
        robotangle.Angle5 = robotangle.Angle5 * 180.0 / PI;
        robotangle.Angle6 = robotangle.Angle6 * 180.0 / PI;
        return robotangle;
    }
    else
    {
        // 第二种解:
        robotangle.Angle3 = temp3b;
        // printf("temp3b\r\n");

        p23 = pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2);
        s23 = -(robotposition->pz * (dhparam[2] + dhparam[1] * cos(robotangle.Angle3)) + (cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]) * (dhparam[3] - dhparam[1] * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2));
        c23 = ((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]) * (dhparam[2] + dhparam[1] * cos(robotangle.Angle3)) - robotposition->pz * (dhparam[3] - dhparam[1] * sin(robotangle.Angle3))) / (pow(robotposition->pz, 2) + pow((cos(robotangle.Angle1) * robotposition->px + sin(robotangle.Angle1) * robotposition->py - dhparam[0]), 2));
        temp23 = atan2(s23, c23);
        // printf("temp23=%f\r\n", temp23 / PI * 180.0);
        if ((temp23 - robotangle.Angle3) > PI)
        {
            robotangle.Angle2 = (temp23 - robotangle.Angle3) - 2 * PI;

            // printf("temp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
        }
        else
        {
            robotangle.Angle2 = temp23 - robotangle.Angle3;
            // printf("btemp2=%f\r\n", robotangle.Angle2 / PI * 180.0);
        }

        s4 = cos(robotangle.Angle1) * robotposition->ay - sin(robotangle.Angle1) * robotposition->ax;
        c4 = sin(robotangle.Angle2 + robotangle.Angle3) * robotposition->az - cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ax - sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) * robotposition->ay;
        // printf("23=%f,s4=%f,c4=%f\r\n", robotangle.Angle3 / PI * 180, s4, c4);
        robotangle.Angle4 = atan2(s4, c4);

        s5 = robotposition->az * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->ax * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle1) * sin(robotangle.Angle4)) - robotposition->ay * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1));
        c5 = -robotposition->ax * (sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) - robotposition->ay * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->az * cos(robotangle.Angle2 + robotangle.Angle3);
        robotangle.Angle5 = atan2(s5, c5);
        if (abs(robotangle.Angle5) < 1e-5)
        {
            // printf("Angle5=%f,Angle4=%f\r\n",robotangle.Angle5 / PI * 180.0,robotangle.Angle4 / PI * 180.0);
            // robotangle.Angle4 = lastrobotangle.Angle4 / 180.0 * PI;
        }
        c6 = robotposition->nx * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + sin(robotangle.Angle4) * sin(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle2 + robotangle.Angle3) * cos(robotangle.Angle1)) +
             robotposition->ny * (cos(robotangle.Angle5) * (cos(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle4) * cos(robotangle.Angle1)) - sin(robotangle.Angle5) * sin(robotangle.Angle1) * sin(robotangle.Angle2 + robotangle.Angle3)) +
             robotposition->nz * (-cos(robotangle.Angle5) * cos(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - sin(robotangle.Angle5) * cos(robotangle.Angle2 + robotangle.Angle3));
        s6 = robotposition->nz * sin(robotangle.Angle4) * sin(robotangle.Angle2 + robotangle.Angle3) - robotposition->nx * (sin(robotangle.Angle4) * cos(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) - cos(robotangle.Angle4) * sin(robotangle.Angle1)) - robotposition->ny * (sin(robotangle.Angle4) * sin(robotangle.Angle1) * cos(robotangle.Angle2 + robotangle.Angle3) + cos(robotangle.Angle4) * cos(robotangle.Angle1));
        robotangle.Angle6 = atan2(s6, c6);
        // printf("%f,%f\r\n", s6, c6);
        // printf("-----------\r\n");
        // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle1 / PI * 180, robotangle.Angle2 / PI * 180, robotangle.Angle3 / PI * 180);
        // printf("%0.6f\r\n%0.6f\r\n%0.6f\r\n", robotangle.Angle4 / PI * 180, robotangle.Angle5 / PI * 180, robotangle.Angle6 / PI * 180);
        if (abs(robotangle.Angle1 - lastrobotangle->Angle1 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle2 - lastrobotangle->Angle2 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle3 - lastrobotangle->Angle3 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle4 - lastrobotangle->Angle4 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle5 - lastrobotangle->Angle5 / 180.0 * PI) < tolrance &&
            abs(robotangle.Angle6 - lastrobotangle->Angle6 / 180.0 * PI) < tolrance)
        {
            // printf("SECOND RESULT\r\n");
            robotangle.Angle1 = robotangle.Angle1 * 180.0 / PI;
            robotangle.Angle2 = robotangle.Angle2 * 180.0 / PI;
            robotangle.Angle3 = robotangle.Angle3 * 180.0 / PI;
            robotangle.Angle4 = robotangle.Angle4 * 180.0 / PI;
            robotangle.Angle5 = robotangle.Angle5 * 180.0 / PI;
            robotangle.Angle6 = robotangle.Angle6 * 180.0 / PI;

            return robotangle;
        }
        else
        {
            robotangle.Angle1 = lastrobotangle->Angle1;
            robotangle.Angle2 = lastrobotangle->Angle2;
            robotangle.Angle3 = lastrobotangle->Angle3;
            robotangle.Angle4 = lastrobotangle->Angle4;
            robotangle.Angle5 = lastrobotangle->Angle5;
            robotangle.Angle6 = lastrobotangle->Angle6;
            PRINTF("INVERSE ERROR!\r\n");
        }
    }
    return robotangle;
}
/**
 * @brief 输入旋转角度θ和起始角度θ_start，计算椭圆的(x,y)坐标和极径R
 * @param input 输入参数（a, b, theta, theta_start）
 * @param output 输出结果（x, y, R）
 * @return 错误码：1=成功，0=参数错误（a<=b），2=参数错误（a或b非正数）
 */
EllipseOutput calc_ellipse_point(EllipseInput input)
{
    EllipseOutput output;
    // 1. 参数合法性校验
    if (input.a <= 0 || input.b <= 0)
    {
        PRINTF("param error!a=%f,b=%f\n", input.a, input.b);
        output.flag = 0;
        return output;
    }
    if (input.a <= input.b)
    {
        PRINTF("param error!a=%f,b=%f\n", input.a, input.b);
        output.flag = 0;
        return output;
    }

    // 2. 计算总角度（起始角度 + 旋转角度）
    double theta_total = input.theta_start / 180.0 * PI;

    // 3. 计算三角函数值（cosθ_total, sinθ_total）
    double cos_theta = cos(theta_total);
    double sin_theta = sin(theta_total);

    // 4. 计算极径R（椭圆极径公式）
    double R = input.a * input.b / sqrt(pow(input.b * cos_theta, 2) + pow(input.a * sin_theta, 2));

    // 5. 计算直角坐标(x,y)（极坐标转直角坐标）
    double x = R * cos_theta;
    double y = R * sin_theta;

    // 6. 赋值输出结果
    output.x = x;
    output.y = y;
    output.R = R;
    output.flag = 1;
    return output; // 计算成功
}
/**
 * @brief 计算椭圆周长（辛普森积分法，100步）
 * @param a 长半轴
 * @param b 短半轴
 * @return 椭圆周长
 */
double ellipse_perimeter(double a, double b)
{
    // 椭圆周长缓存（静态变量，只计算一次）
    double cached_perimeter = 0.0;
    double cached_a = 0.0;
    double cached_b = 0.0;
    // 如果参数未变，直接返回缓存值
    if (a == cached_a && b == cached_b && cached_perimeter > 0)
    {
        return cached_perimeter;
    }

    // 辛普森积分参数
    const int n = 100;
    const double h = (2 * PI) / n;
    double s = 0.0;

    // 被积函数：椭圆弧长元素
#define INTEGRAND(theta) sqrt(a * a * sin(theta) * sin(theta) + b * b * cos(theta) * cos(theta))

    // 辛普森积分计算
    s += INTEGRAND(0.0) + INTEGRAND(2 * PI);
    for (int i = 1; i < n; i += 2)
    {
        s += 4 * INTEGRAND(i * h);
    }
    for (int i = 2; i < n; i += 2)
    {
        s += 2 * INTEGRAND(i * h);
    }

    // 计算结果并缓存
    cached_perimeter = (h / 3) * s;
    cached_a = a;
    cached_b = b;

    return cached_perimeter;
}

/**
 * @brief 计算从X+轴到指定角度的椭圆弧长
 * @param a 长半轴
 * @param b 短半轴
 * @param theta_rad 角度（弧度）
 * @return 弧长
 */
double fast_ellipse_arc_length(double a, double b, double theta_rad)
{
    const int n = 100;
    const double h = theta_rad / n;
    double s = 0.0;

    // 被积函数（同椭圆周长计算）
#define INTEGRAND(theta) sqrt(a * a * sin(theta) * sin(theta) + b * b * cos(theta) * cos(theta))

    // 辛普森积分计算
    s += INTEGRAND(0.0) + INTEGRAND(theta_rad);
    for (int i = 1; i < n; i += 2)
    {
        s += 4 * INTEGRAND(i * h);
    }
    for (int i = 2; i < n; i += 2)
    {
        s += 2 * INTEGRAND(i * h);
    }

    return (h / 3) * s;
}

/**
 * @brief 已知椭圆弧长，反向求解等效角度（0~360度）
 * @param a 长半轴
 * @param b 短半轴
 * @param target_arc 目标弧长
 * @return 等效角度（度），失败返回-1
 */
double arc_length_to_angle(double a, double b, double perimeter, double target_arc)
{
    // 输入验证
    if (a <= 0 || b <= 0 || target_arc < 0)
    {
        PRINTF("param error\n");
        return -1;
    }

    // 特殊情况：弧长为0
    if (fabs(target_arc) < 1e-12)
    {
        return 0.0;
    }

    // 步骤1：获取椭圆周长（自动缓存）
    // const double perimeter = ellipse_perimeter(a, b);

    // 步骤2：处理超过周长的弧长（周期性）
    const double remainder_arc = fmod(target_arc, perimeter);

    // 步骤3：精准初始估算（缩小搜索区间）
    const double avg_r = (a + b) / 2;
    const double eccentricity = (a > b) ? sqrt(1 - (b * b) / (a * a)) : sqrt(1 - (a * a) / (b * b));
    double theta0 = remainder_arc / (avg_r * (1 - (eccentricity * eccentricity) / 4));

    // 限制搜索区间在[0, 2π]内
    double low = fmax(0.0, theta0 - 0.05);
    double high = fmin(2 * PI, theta0 + 0.05);

    // 步骤4：二分法搜索（最多15次迭代，足够收敛）
    const int max_iter = 15;
    for (int i = 0; i < max_iter; i++)
    {
        const double mid = (low + high) / 2;
        const double current_arc = fast_ellipse_arc_length(a, b, mid);
        const double diff = current_arc - remainder_arc;

        if (fabs(diff) < 1e-7)
        { // 弧长误差小于1e-7，满足高精度需求
            break;
        }
        else if (diff < 0)
        {
            low = mid;
        }
        else
        {
            high = mid;
        }
    }

    // 步骤5：弧度转角度，确保结果在[0, 360)范围内
    double angle_deg = (low + high) / 2 * 180 / PI;
    return fmod(angle_deg, 360.0);
}

/**
 * @brief 高斯-勒让德20点积分（计算[theta0, theta1]区间的弧长）
 * @param ec 椭圆计算器实例
 * @param theta0 积分下限（弧度）
 * @param theta1 积分上限（弧度）
 * @return 区间弧长（mm）
 */
static double integrate_arc_length(const EllipseCalculator *ec, double theta0, double theta1)
{
    static const double x[] = {
        -0.9931285991850949, -0.9639719272779138, -0.9122344282513259,
        -0.8391169718222188, -0.7463319064601508, -0.6360536807265150,
        -0.5108670019508271, -0.3737060887154196, -0.2277858511416451,
        -0.0765265211334973, 0.0765265211334973, 0.2277858511416451,
        0.3737060887154196, 0.5108670019508271, 0.6360536807265150,
        0.7463319064601508, 0.8391169718222188, 0.9122344282513259,
        0.9639719272779138, 0.9931285991850949};
    static const double w[] = {
        0.0176140071391521, 0.0406014298003869, 0.0626720483341091,
        0.0832767415767048, 0.1019301198172404, 0.1181945319615184,
        0.1316886384491766, 0.1420961093183821, 0.1491729864726037,
        0.1527533871307258, 0.1527533871307258, 0.1491729864726037,
        0.1420961093183821, 0.1316886384491766, 0.1181945319615184,
        0.1019301198172404, 0.0832767415767048, 0.0626720483341091,
        0.0406014298003869, 0.0176140071391521};

    double h = (theta1 - theta0) / 2.0;
    double c = (theta1 + theta0) / 2.0;
    double integral = 0.0;

    for (int i = 0; i < 20; i++)
    {
        double t = c + h * x[i];
        double cos_t = cos(t);
        integral += w[i] * (ec->a * sqrt(1.0 - ec->e2 * cos_t * cos_t));
    }

    return h * integral;
}

/**
 * @brief 拉马努金公式计算椭圆周长（O(1)，误差<0.01%）
 * @param a 长半轴
 * @param b 短半轴
 * @return 椭圆周长（mm）
 */
static double calculate_perimeter(double a, double b)
{
    double sum_ab = a + b;
    double term = (3 * a + b) * (a + 3 * b);
    return PI * (3 * sum_ab - sqrt(term));
}

/**
 * @brief 初始化椭圆计算器（预计算查找表，仅需调用一次）
 * @param ec 椭圆计算器实例指针
 * @param a 长半轴（mm）
 * @param b 短半轴（mm）
 * @param sample_cnt 查找表采样点数（建议≥5万，默认10万，精度≈1e-5弧度）
 * @return 0=成功，-1=参数错误，-2=内存分配失败
 */
int EllipseCalculator_Init(EllipseCalculator *ec, double a, double b, size_t sample_cnt)
{
    if (ec == NULL || a <= 0 || b <= 0 || sample_cnt < 2)
    {
        return -1;
    }

    ec->a = (a >= b) ? a : b;
    ec->b = (a >= b) ? b : a;
    ec->e2 = 1.0 - (ec->b * ec->b) / (ec->a * ec->a);
    ec->perimeter = calculate_perimeter(ec->a, ec->b);
    ec->sample_cnt = sample_cnt;

    ec->theta_lut = (double *)malloc(sample_cnt * sizeof(double));
    ec->s_lut = (double *)malloc(sample_cnt * sizeof(double));
    if (ec->theta_lut == NULL || ec->s_lut == NULL)
    {
        free(ec->theta_lut);
        free(ec->s_lut);
        return -2;
    }

    double d_theta = 2 * PI / (sample_cnt - 1);
    double s_accum = 0.0;

    ec->theta_lut[0] = 0.0;
    ec->s_lut[0] = 0.0;

    for (size_t i = 1; i < sample_cnt; i++)
    {
        ec->theta_lut[i] = i * d_theta;
        s_accum += integrate_arc_length(ec, ec->theta_lut[i - 1], ec->theta_lut[i]);
        ec->s_lut[i] = s_accum;
    }

    ec->s_lut[sample_cnt - 1] = ec->perimeter; // 修正闭环误差

    return 0;
}

/**
 * @brief 二分查找（查找s在弧长查找表中的位置）
 * @param ec 椭圆计算器实例
 * @param s 目标弧长（mm）
 * @return 第一个≥s的元素索引
 */
static size_t binary_search(const EllipseCalculator *ec, double s)
{
    size_t low = 0;
    size_t high = ec->sample_cnt - 1;

    while (low < high)
    {
        size_t mid = low + (high - low) / 2;
        if (ec->s_lut[mid] < s)
        {
            low = mid + 1;
        }
        else
        {
            high = mid;
        }
    }

    return low;
}

/**
 * @brief 弧长→角度映射（超出周长部分从0开始循环计算）
 * @param ec 椭圆计算器实例
 * @param s 弧长（mm，支持任意正负值）
 * @return 对应的角度（弧度，范围[0, 2π)）
 */
double EllipseCalculator_ArcLengthToAngle(EllipseCalculator *ec, double s)
{
    assert(ec != NULL && ec->theta_lut != NULL && ec->s_lut != NULL);

    double perimeter = ec->perimeter;
    if (perimeter <= 0.0)
        return 0.0;

    //(新增)累计角度逻辑
    double n = floor(s / perimeter); // 计算完整周数（n为整数，正负均可）

    // 核心修改：弧长对周长取模，支持正负值循环（超出部分从0开始）
    s = fmod(s, perimeter);
    if (s < 0.0)
    { // 处理负弧长（转为正弧长，例如 s=-10 → perimeter-10）
        s += perimeter;
    }

    // 现在s ∈ [0, perimeter)，直接查找插值
    size_t idx = binary_search(ec, s);
    if (idx == 0)
        return 0.0; // s=0时返回0弧度

    double s0 = ec->s_lut[idx - 1];
    double s1 = ec->s_lut[idx];
    double theta0 = ec->theta_lut[idx - 1];
    double theta1 = ec->theta_lut[idx];

    return (theta0 + (s - s0) * (theta1 - theta0) / (s1 - s0) + 2 * PI * n) / PI * 180.0;
}

/**
 * @brief 获取椭圆周长
 * @param ec 椭圆计算器实例
 * @return 周长（mm）
 */
double EllipseCalculator_GetPerimeter(const EllipseCalculator *ec)
{
    assert(ec != NULL);
    return ec->perimeter;
}

/**
 * @brief 销毁椭圆计算器（释放内存，避免泄漏）
 * @param ec 椭圆计算器实例
 */
void EllipseCalculator_Destroy(EllipseCalculator *ec)
{
    if (ec == NULL)
        return;
    free(ec->theta_lut);
    free(ec->s_lut);
    ec->theta_lut = NULL;
    ec->s_lut = NULL;
}

RobotQuationPos stopPlan_zt(RobotQuationPos nowp, RobotQuationPos endp, double nowvel, double remainingdisance)
{
    //    printf("stopPlan nowp:%f,%f,%f\r\n",nowp.x,nowp.y,nowp.z);
    //    printf("stopPlan endp:%f,%f,%f\r\n",endp.x,endp.y,endp.z);
    //    printf("stopPlan nowvel:%f,remainingdisance:%f\r\n",nowvel,remainingdisance);
    double stoptime = 0.3;
    double deceleration = -nowvel / stoptime;
    double stopdistance = 0.5 * deceleration * stoptime * stoptime + nowvel * stoptime;
    //     printf("stopPlan nowvel:%f,remainingdisance:%f,stopdistance:%f\r\n",nowvel,remainingdisance,stopdistance);
    RobotQuationPos stopPos;
    stopPos.x = nowp.x + (endp.x - nowp.x) * (stopdistance / remainingdisance);
    stopPos.y = nowp.y + (endp.y - nowp.y) * (stopdistance / remainingdisance);
    stopPos.z = nowp.z + (endp.z - nowp.z) * (stopdistance / remainingdisance);

    // 2. 计算比例值，并添加边界判断和除零保护
    float bili = 0.0f; // 初始化比例值
    if (fabs(remainingdisance) < 1e-6)
    { // 避免除零错误（剩余距离接近0）
        PRINTF("remainingdisance is zero, set bili to 0!\r\n");
        bili = 0.0f;
    }
    else
    {
        bili = (float)(stopdistance / remainingdisance);
        if (bili > 1.0f)
        { // 比值大于1时强制设为1
            bili = 1.0f;
        }
        // 比值小于0时设为0（防止负数导致位置异常）
        if (bili < 0.0f)
        {
            bili = 0.0f;
        }
    }

    // 3. 调用nextQuationPostion计算姿态
    RobotQuationPos tempResult = nextQuationPostion(nowp, endp, bili);

    // 4. 将计算得到的姿态赋值给stopPos
    stopPos.ox = tempResult.ox;
    stopPos.oy = tempResult.oy;
    stopPos.oz = tempResult.oz;
    stopPos.ow = tempResult.ow;
    // PRINTF("stopPlan_zt stopPos %f %f %f %f\r\n", stopPos.ox, stopPos.oy, stopPos.oz, stopPos.ow);
    // PRINTF("stopPlan_zt nowp %f %f %f %f\r\n", nowp.ox, nowp.oy, nowp.oz, nowp.ow);
    // PRINTF("rate:%f %f %f\r\n", bili, stopdistance, remainingdisance);
    return stopPos;
}

/// @brief 六轴串联movj规划
/// @param currentaxisdec 当前六轴的dec
/// @param mp 各轴传动参数
/// @param jointAngle_endpos 目标位置的关节角度
/// @param vel 速度(单位:dec/s)
/// @return 返回各轴匹配指令参数
sixvelplan_ sixRobot_movj(int currentaxisdec[6], motorparam_ mp[6], RobotAngle jointAngle_endpos, double vel)
{
    sixvelplan_ data;
    PRINTF("SixMOVJ dec: [0]=%d,[1]=%d,[2]=%d,[3]=%d,[4]=%d,[5]=%d\n",
           currentaxisdec[0], currentaxisdec[1], currentaxisdec[2], currentaxisdec[3], currentaxisdec[4], currentaxisdec[5]);
    PRINTF("SixMOVJ angle: [0]=%f,[1]=%f,[2]=%f,[3]=%f,[4]=%f,[5]=%f,vel=%d\n",
           jointAngle_endpos.Angle1, jointAngle_endpos.Angle2, jointAngle_endpos.Angle3,
           jointAngle_endpos.Angle4, jointAngle_endpos.Angle5, jointAngle_endpos.Angle6, vel);

    // 关节角转dec
    int dec_endpos[6];
    crobotJointToDec(&jointAngle_endpos, mp, dec_endpos);
    int jointdec_diff[6];
    int max_dif = 0;
    int max_num = 0;
    for (int i = 0; i < 6; i++)
    {
        jointdec_diff[i] = dec_endpos[i] - currentaxisdec[i];
        if (abs(jointdec_diff[i]) > max_dif)
        {
            max_dif = abs(jointdec_diff[i]);
            max_num = i;
        }
    }
    double vel_d = (double)vel; // 参考速度转double
    vel_d = (double)vel * (mp[max_num].transmission * 1000.0);
    double tmax = 0;
    int plan_vel[6];
    // 1.检查参数
    if (vel == 0)
    {
        memset(plan_vel, 0, sizeof(int) * 6);
        tmax = 0.0;
        for (int i = 0; i < 6; i++)
        {
            data.velocity[i] = plan_vel[i];
            data.targetposition[i] = 0;
        }

        return data;
    }
    else
    {
        // 2. 转换为double类型计算（避免整数精度丢失）

        double dec_diff_d[6] = {0.0}; // 距离转double
        double t_array[6] = {0.0};    // 各轴理论运行时间（s）
        tmax = 0.0;

        // 3. 计算各轴理论运行时间，并找到最长时间tmax
        int all_zero_flag = 1; // 标记是否所有距离都为0
        for (int i = 0; i < 6; i++)
        {
            dec_diff_d[i] = (double)jointdec_diff[i]; // int转double
            double dec_abs = abs(dec_diff_d[i]);      // 距离取绝对值

            // 若当前轴距离非0，标记非全零
            if (dec_abs > 1e-9 || dec_abs < -1e-9)
            {
                all_zero_flag = 0;
                t_array[i] = dec_abs / vel_d; // 计算理论运行时间
                // 更新最长运行时间
                if (t_array[i] > tmax)
                {
                    tmax = t_array[i];
                }
            }
            else
            {
                t_array[i] = 0.0; // 距离为0，时间为0
            }
        }

        // 4. 边界校验：所有距离都为0
        if (all_zero_flag)
        {
            memset(plan_vel, 0, sizeof(int) * 6);
            tmax = 0.0;
            for (int i = 0; i < 6; i++)
            {
                data.velocity[i] = plan_vel[i];
                data.targetposition[i] = 0;
            }
            return data;
        }

        // 5. 计算各轴目标速度（double高精度计算，最后转int）
        for (int i = 0; i < 6; i++)
        {
            // 目标速度 = 轴距离 / 最长时间（保留正负号，代表运动方向）
            double vel_i_d = dec_diff_d[i] / tmax;
            // 四舍五入转int
            plan_vel[i] = (int)(vel_i_d + 0.5);
            data.velocity[i] = plan_vel[i];
            data.targetposition[i] = abs(jointdec_diff[i]);
        }
    }
    return data;
}