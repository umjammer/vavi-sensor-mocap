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
  fflush(stdout);
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
  fflush(stdout);
}

// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie) {
  printf("Pose %s detected for user %d\n", strPose, nId);
  fflush(stdout);
  g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
  g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}

// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie) {
  printf("Calibration started for user %d\n", nId);
  fflush(stdout);
}

// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie) {
  if (bSuccess)	{
    // Calibration succeeded
    printf("Calibration complete, start tracking user %d\n", nId);
    fflush(stdout);
    g_UserGenerator.GetSkeletonCap().StartTracking(nId);
  }	else {
    // Calibration failed
    printf("Calibration failed for user %d\n", nId);
    fflush(stdout);
    if (g_bNeedPose) {
      g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
    } else {
      g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
  }
}

extern xn::UserGenerator g_UserGenerator;

void inject(XnUserID player, XnSkeletonJoint eJoint1, XnSkeletonJoint eJoint2,
  JNIEnv *env, jobjectArray array1, int index) {

  if (!g_UserGenerator.GetSkeletonCap().IsTracking(player)) {
//printf("not tracked!\n");
//fflush(stdout);
    return;
  }

  XnSkeletonJointPosition joint1, joint2;
  g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint1, joint1);
  g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint2, joint2);

  if (joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5) {
//printf("low confidence\n");
//fflush(stdout);
    return;
  }

  jfloatArray array2 = (jfloatArray) env->GetObjectArrayElement(array1, index);
  float* data = env->GetFloatArrayElements(array2, NULL);

  data[0] = eJoint1;
  data[1] = joint1.position.X; 
  data[2] = joint1.position.Y;
  data[3] = joint1.position.Z;
  data[4] = eJoint2;
  data[5] = joint2.position.X;
  data[6] = joint2.position.Y;
  data[7] = joint2.position.Z;
  data[8] = player;
/*
printf("%d: %d: %.2f, %.2f, %.2f %d: %.2f, %.2f, %.2f\n",
(int) data[8], 
(int) data[0], 
data[1], 
data[2], 
data[3], 
(int) data[4], 
data[5], 
data[6], 
data[7]);
fflush(stdout);
*/
  env->SetFloatArrayRegion(array2, 0, 9, data);
  env->ReleaseFloatArrayElements(array2, data, NULL);
}

#define XML_PATH ".openni/nite_config.xml"

#define CHECK_RC(nRetVal, what)									          \
  if (nRetVal != XN_STATUS_OK) {								          \
	fprintf(stderr, "%s failed: %s\n", what, xnGetStatusString(nRetVal)); \
    fflush(stderr);                                                       \
	return nRetVal;												          \
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
//fprintf(stdout, "config: %s\n", bytes);
//fflush(stdout);
  nRetVal = g_Context.InitFromXmlFile(bytes, &errors);
  if (nRetVal == XN_STATUS_NO_NODE_PRESENT) {
    XnChar strError[1024];
    errors.ToString(strError, 1024);
    fprintf(stderr, "%s\n", strError);
    fflush(stderr);
    return nRetVal;
  } else if (nRetVal != XN_STATUS_OK) {
    fprintf(stderr, "Open failed: %s\n", xnGetStatusString(nRetVal));
    fflush(stderr);
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
    fprintf(stderr, "Supplied user generator doesn't support skeleton\n");
    fflush(stderr);
    return 1;
  }
  g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
  g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart, UserCalibration_CalibrationEnd, NULL, hCalibrationCallbacks);

  if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration()) {
    g_bNeedPose = TRUE;
    if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)) {
      fprintf(stderr, "Pose required, but not supported\n");
      fflush(stderr);
      return 1;
    }
    g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected, NULL, NULL, hPoseCallbacks);
    g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
  }

  g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

  nRetVal = g_Context.StartGeneratingAll();
  CHECK_RC(nRetVal, "StartGenerating");

//printf("here we go. 1\n");
//fflush(stdout);
  return 0;
}

/*
 * Class:     vavi_sensor_mocap_macbook_NiteMocap
 * Method:    inject
 * Signature: ([[[F)V
 */
JNIEXPORT void JNICALL Java_vavi_sensor_mocap_macbook_NiteMocap_inject
  (JNIEnv *env, jobject obj, jobjectArray result) {

  xn::SceneMetaData sceneMD;
  xn::DepthMetaData depthMD;

  // Read next available data
  g_Context.WaitAndUpdateAll();

  // Process the data
  g_DepthGenerator.GetMetaData(depthMD);
  g_UserGenerator.GetUserPixels(0, sceneMD);

  XnUserID aUsers[15];
  XnUInt16 nUsers = 15;
  g_UserGenerator.GetUsers(aUsers, nUsers);
  for (int i = 0; i < nUsers; i++) {
    // Tracking
//printf("%d\n", aUsers[i]);
//fflush(stdout);

    if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])) {
      // Tracking
      printf("%d - Tracking\n", aUsers[i]);
      fflush(stdout);
    } else if (g_UserGenerator.GetSkeletonCap().IsCalibrating(aUsers[i])) {
      // Calibrating
      printf("%d - Calibrating...\n", aUsers[i]);
      fflush(stdout);
    } else {
      // Nothing
      printf("%d - Looking for pose\n", aUsers[i]);
      fflush(stdout);
    }
        
    if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])) {
      jobjectArray array1 = (jobjectArray) env->GetObjectArrayElement(result, i);

      inject(aUsers[i], XN_SKEL_HEAD, XN_SKEL_NECK, env, array1, 0);
            
      inject(aUsers[i], XN_SKEL_NECK, XN_SKEL_LEFT_SHOULDER, env, array1, 1);
      inject(aUsers[i], XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW, env, array1, 2);
      inject(aUsers[i], XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND, env, array1, 3);
            
      inject(aUsers[i], XN_SKEL_NECK, XN_SKEL_RIGHT_SHOULDER, env, array1, 4);
      inject(aUsers[i], XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW, env, array1, 5);
      inject(aUsers[i], XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND, env, array1, 6);
            
      inject(aUsers[i], XN_SKEL_LEFT_SHOULDER, XN_SKEL_TORSO, env, array1, 7);
      inject(aUsers[i], XN_SKEL_RIGHT_SHOULDER, XN_SKEL_TORSO, env, array1, 8);
            
      inject(aUsers[i], XN_SKEL_TORSO, XN_SKEL_LEFT_HIP, env, array1, 9);
      inject(aUsers[i], XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE, env, array1, 10);
      inject(aUsers[i], XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT, env, array1, 11);
            
      inject(aUsers[i], XN_SKEL_TORSO, XN_SKEL_RIGHT_HIP, env, array1, 12);
      inject(aUsers[i], XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE, env, array1, 13);
      inject(aUsers[i], XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT, env, array1, 14);
            
      inject(aUsers[i], XN_SKEL_LEFT_HIP, XN_SKEL_RIGHT_HIP, env, array1, 15);
    }
  }
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
 * Method:    inject
 * Signature: ([[[F)V
 */
JNIEXPORT void JNICALL Java_vavi_sensor_mocap_macbook_NiteMocap_testInject
  (JNIEnv *env, jobject obj, jobjectArray result) {
  
  for (int i = 0; i < 15; i++) {
    jobjectArray array1 = (jobjectArray) env->GetObjectArrayElement(result, i);

    for (int i = 0; i < 15; i++) {
      jfloatArray array2 = (jfloatArray) env->GetObjectArrayElement(array1, 0);
      float* data = env->GetFloatArrayElements(array2, NULL);
      data[0] = 1;
      data[1] = 2;
      data[2] = 3;
      data[3] = 4;
      data[4] = 5;
      data[5] = 6;
      data[6] = 7;
      data[7] = 8;
      data[8] = 9;
      env->SetFloatArrayRegion(array2, 0, 9, data);
      env->ReleaseFloatArrayElements(array2, data, NULL);
    }
  }
}

/* */