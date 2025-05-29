#ifndef Ava_h
#define Ava_h

#include <Herkulex.h>

#define MOTOR_COUNT 27

// Motor IDs and positions for Ava
// 23-24 MQP

struct Motor{
  int hexID;
  int minPos;
  int maxPos;
  int homePos;
  HerkulexModel model;
  String description;
};

extern Motor
    Left_Elbow,
    Left_Arm_Rotator, Left_Arm_Abductor, Right_Leg_Rotator, Right_Ankle,                                                   // 1-5
    Right_Arm_Abductor, Left_Shoulder, Right_Leg_Abductor_Side_To_Side, Left_Leg_Abductor_Side_To_Side, Right_Arm_Rotator, // 6-10
    Right_Elbow, Left_Knee, Left_Ankle, Left_Leg_Rotator, Right_Shoulder,                                                  // 11-15
    Back_Chest, Front_Chest, Hips_Rotate_Upper_Body, Right_Knee, Hips_Lean_Side_To_Side,                                   // 17 - 21
    Hips_Bend_Over, Right_Wrist_Abductor, Left_Wrist_Abductor, Bottom_Neck, Top_Neck,                                      // 22, 25-28
    Left_Leg_Abductor_Front_To_Back, Right_Leg_Abductor_Front_To_Back;

// Ordered in motors array by id (first value of struct)
// FYI ids 16, 23-24, and 29 are unused
extern int motorsLen;
extern Motor motors[MOTOR_COUNT];

#endif