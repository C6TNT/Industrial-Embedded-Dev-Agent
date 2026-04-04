#ifndef COORDINATETRANS_H
#define COORDINATETRANS_H
extern	"C"
{
    void versionData();
    void matrix_inverse(double(*a)[3], double(*b)[3]);
    void matrix_inverse4(double(*a)[4], double(*b)[4]);
    void multi_axis_kinematics_to_six(double* ida, double* tool, double* spacePosture);
    void calculate_vertical(double* a, double* b, double* c, double* d, double * rpy);
    void calibrate_origin(double* p, double* cam_delta, double *cam_comp, double *cam, double* outp);
    void calibrate_origin2(double* p, double* cam_delta, double *cam_comp, double *cam, double *camRot, double* outp);
    void rotation_angle(double* p1, double* p2, double* outp1);
    void trans_2to1(double* t_pos2, double* sita, double* t_pos1);
    void workpiece_to_robot_coordinate(double* workpiece_1_origin, double* workpiece_1_position, double* w_trc_out);
    void robotTo_workpiece2(double* robot_space_pos, double* s_wp2_origin, double* sita, double* s_wp2_pos);
    void trans_to_workpiece1(double* origin_pos, double* robot_space_pos, double* s_wp1);
    void adjust_matrix(double* p1, double* p2, double* p_matrix);
    void camera_to_robot_trans(double* p_matrix, double* camera, double* camera_pos, double* robot_pos);
    void laser_to_robot_trans(double* lp_matrix, double* laser_pos, double* robot_pos);
    void robot_to_laser_trans(double* lp_matrix, double* robot_pos, double* laser_pos);
    void cad_to_work2_trans_matrix(double* p_a0, double* p_b0, double* p_a1, double* p_b1, double* solution);
    void cad_to_work2_coordinate(double* cad_pos, double* trans_matrix, double* deep, double* w2_pos);
    void gt_to_work2_trans_matrix(double* p_a0, double* p_b0, double* p_a1, double* p_b1, double* solution);
    void gt_to_work2_coordinate(double* gt_pos, double* trans_matrix, double* w2_pos);
}
#endif // COORDINATETRANS_H
