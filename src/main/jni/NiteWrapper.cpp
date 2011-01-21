/*****************************************************************************
*                                                                            *
*  OpenNI 1.0 Alpha                                                          *
*  Copyright (C) 2010 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  OpenNI is free software: you can redistribute it and/or modify            *
*  it under the terms of the GNU Lesser General Public License as published  *
*  by the Free Software Foundation, either version 3 of the License, or      *
*  (at your option) any later version.                                       *
*                                                                            *
*  OpenNI is distributed in the hope that it will be useful,                 *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
*  GNU Lesser General Public License for more details.                       *
*                                                                            *
*  You should have received a copy of the GNU Lesser General Public License  *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.            *
*                                                                            *
*****************************************************************************/

#include "vavi_sensor_mocap_macbook_NiteMocap.h"

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <stdio.h>

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
xn::Context g_Context;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;

XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
  printf("New User %d\n", nId);
  // New user found
  if (g_bNeedPose) {
    g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
  }	else {
    g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
  }
}

// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
  printf("Lost user %d\n", nId);
}

// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie) {
  printf("Pose %s detected for user %d\n", strPose, nId);
  g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
  g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}

// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie) {
  printf("Calibration started for user %d\n", nId);
}

// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie) {
  if (bSuccess)	{
    // Calibration succeeded
    printf("Calibration complete, start tracking user %d\n", nId);
    g_UserGenerator.GetSkeletonCap().StartTracking(nId);
  }	else {
    // Calibration failed
    printf("Calibration failed for user %d\n", nId);
    if (g_bNeedPose) {
      g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
    } else {
      g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
  }
}

extern xn::UserGenerator g_UserGenerator;

float result[15][16][9];

void fillResult(int index, XnUserID player, int pattern, XnSkeletonJoint eJoint1, XnSkeletonJoint eJoint2) {
  if (!g_UserGenerator.GetSkeletonCap().IsTracking(player)) {
    printf("not tracked!\n");
    return;
  }

  XnSkeletonJointPosition joint1, joint2;
  g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint1, joint1);
  g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint2, joint2);

  if (joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5) {
    return;
  }

  result[index][pattern][0] = eJoint1;
  result[index][pattern][1] = joint1.position.X; 
  result[index][pattern][2] = joint1.position.Y;
  result[index][pattern][3] = joint1.position.Z;
  result[index][pattern][4] = eJoint2;
  result[index][pattern][5] = joint2.position.X;
  result[index][pattern][6] = joint2.position.Y;
  result[index][pattern][7] = joint2.position.Z;
  result[index][pattern][8] = player;
}

#define XML_PATH ".openni/nite_config.xml"

#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
		return nRetVal;												\
	}

/*
 * Class:     vavi_sensor_mocap_macbook_NiteMocap
 * Method:    init
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_vavi_sensor_mocap_macbook_NiteMocap_init
  (JNIEnv *env, jobject obj) {

  XnStatus nRetVal = XN_STATUS_OK;

  xn::EnumerationErrors errors;
  char bytes[1024];
  snprintf(bytes, 1024, "%s/%s", getenv("HOME"), XML_PATH);
  printf("config: %s\n", bytes);
  nRetVal = g_Context.InitFromXmlFile(bytes, &errors);
  if (nRetVal == XN_STATUS_NO_NODE_PRESENT) {
    XnChar strError[1024];
    errors.ToString(strError, 1024);
    printf("%s\n", strError);
    return nRetVal;
  } else if (nRetVal != XN_STATUS_OK) {
    printf("Open failed: %s\n", xnGetStatusString(nRetVal));
    return nRetVal;
  }

  nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
  CHECK_RC(nRetVal, "Find depth generator");
  nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
  if (nRetVal != XN_STATUS_OK) {
    nRetVal = g_UserGenerator.Create(g_Context);
    CHECK_RC(nRetVal, "Find user generator");
  }

  XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;
  if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON)) {
    printf("Supplied user generator doesn't support skeleton\n");
    return 1;
  }
  g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
  g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart, UserCalibration_CalibrationEnd, NULL, hCalibrationCallbacks);

  if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration()) {
    g_bNeedPose = TRUE;
    if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)) {
      printf("Pose required, but not supported\n");
      return 1;
    }
    g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected, NULL, NULL, hPoseCallbacks);
    g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
  }

  g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

  nRetVal = g_Context.StartGeneratingAll();
  CHECK_RC(nRetVal, "StartGenerating");

  return 0;
}

/*
 * Class:     vavi_sensor_mocap_macbook_NiteMocap
 * Method:    sense
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_vavi_sensor_mocap_macbook_NiteMocap_sense
  (JNIEnv *env, jobject obj) {

  xn::SceneMetaData sceneMD;
  xn::DepthMetaData depthMD;

  // Read next available data
//  g_Context.WaitAndUpdateAll(); // pause

  // Process the data
  g_DepthGenerator.GetMetaData(depthMD);
  g_UserGenerator.GetUserPixels(0, sceneMD);

  XnUserID aUsers[15];
  XnUInt16 nUsers = 15;
  g_UserGenerator.GetUsers(aUsers, nUsers);
  for (int i = 0; i < nUsers; i++) {
    // Tracking
    printf("%d", aUsers[i]);

    if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])) {
      // Tracking
      printf("%d - Tracking", aUsers[i]);
    } else if (g_UserGenerator.GetSkeletonCap().IsCalibrating(aUsers[i])) {
      // Calibrating
      printf("%d - Calibrating...", aUsers[i]);
    } else {
      // Nothing
      printf("%d - Looking for pose", aUsers[i]);
    }
        
    if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])) {
      fillResult(i, aUsers[i], 0, XN_SKEL_HEAD, XN_SKEL_NECK);
            
      fillResult(i, aUsers[i], 1, XN_SKEL_NECK, XN_SKEL_LEFT_SHOULDER);
      fillResult(i, aUsers[i], 2, XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW);
      fillResult(i, aUsers[i], 3, XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND);
            
      fillResult(i, aUsers[i], 4, XN_SKEL_NECK, XN_SKEL_RIGHT_SHOULDER);
      fillResult(i, aUsers[i], 5, XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW);
      fillResult(i, aUsers[i], 6, XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND);
            
      fillResult(i, aUsers[i], 7, XN_SKEL_LEFT_SHOULDER, XN_SKEL_TORSO);
      fillResult(i, aUsers[i], 8, XN_SKEL_RIGHT_SHOULDER, XN_SKEL_TORSO);
            
      fillResult(i, aUsers[i], 9, XN_SKEL_TORSO, XN_SKEL_LEFT_HIP);
      fillResult(i, aUsers[i], 10, XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE);
      fillResult(i, aUsers[i], 11, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT);
            
      fillResult(i, aUsers[i], 12, XN_SKEL_TORSO, XN_SKEL_RIGHT_HIP);
      fillResult(i, aUsers[i], 13, XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE);
      fillResult(i, aUsers[i], 14, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT);
            
      fillResult(i, aUsers[i], 15, XN_SKEL_LEFT_HIP, XN_SKEL_RIGHT_HIP);
    }
  }

  return 0;
}

/*
 * Class:     vavi_sensor_mocap_macbook_NiteMocap
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vavi_sensor_mocap_macbook_NiteMocap_destroy
  (JNIEnv *env, jobject obj) {

  g_Context.Shutdown();
}

/*
 * Class:     vavi_sensor_mocap_macbook_NiteMocap
 * Method:    get
 * Signature: ()[[[F
 */
JNIEXPORT jobjectArray JNICALL Java_vavi_sensor_mocap_macbook_NiteMocap_get
  (JNIEnv *env, jobject obj) {

  // Create the class
  jclass floatClass1 = env->FindClass("[[F");
  jclass floatClass2 = env->FindClass("[F");

  // Create 1st and 2nd array
  jobjectArray array1 = env->NewObjectArray(15, floatClass1, NULL);
  
  for (int i = 0; i < 15; i++) {
    jobjectArray array2 = env->NewObjectArray(16, floatClass2, NULL);

    for (int j = 0; j < 16; j++) {
      // Create 3rd Array
      jfloatArray array3 = env->NewFloatArray(9);

      // Put some data into array3
      env->SetFloatArrayRegion(array3, 0, 9, result[i][j]);

      // Add array 3 to array 2 and array 2 to array 1
      env->SetObjectArrayElement(array2, j, array3);
    }

    env->SetObjectArrayElement(array1, i, array2);
  }

  return array1;
}

/* */