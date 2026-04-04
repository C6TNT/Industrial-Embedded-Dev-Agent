#ifndef PYTHON_H
#define PYTHON_H

void python_init();
void python_set_matrix(int axisNum, double joint_params[][3]);
void python_get_matrix(int axisNum,double *theta,double *matrix);
void python_kinematics(int axisNum, double* target_matrix, double* current_theta, double* solved_theta);
#endif // PYTHON_H
