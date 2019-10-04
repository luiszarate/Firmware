/****************************************************************************
 *
 *   Copyright (C) 2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include <gtest/gtest.h>
#include <ControlMath.hpp>
#include <px4_defines.h>

using namespace matrix;
using namespace ControlMath;

TEST(ControlMathTest, ThrustToAttitude)
{
	Vector3f body = Vector3f(0, 0, 1).normalized();
	Quatf q;
	bodyzToQuaternion(q, body, 0);
	EXPECT_EQ(body, q.dcm_z());
}

TEST(ControlMathTest, LimitTiltUnchanged)
{
	Vector3f body = Vector3f(0, 0, 1).normalized();
	Vector3f body_before = body;
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 45.f);
	EXPECT_EQ(body, body_before);

	body = Vector3f(0, .1f, 1).normalized();
	body_before = body;
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 45.f);
	EXPECT_EQ(body, body_before);
}

TEST(ControlMathTest, LimitTiltOpposite)
{
	Vector3f body = Vector3f(0, 0, -1).normalized();
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 45.f);
	float angle = acosf(body.dot(Vector3f(0, 0, 1)));
	EXPECT_NEAR(angle * M_RAD_TO_DEG_F, 45.f, 1e-4f);
	EXPECT_FLOAT_EQ(body.length(), 1.f);
}

TEST(ControlMathTest, LimitTiltAlmostOpposite)
{
	// This case doesn't trigger corner case handling but is very close to it
	Vector3f body = Vector3f(0.001f, 0, -1.f).normalized();
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 45.f);
	float angle = acosf(body.dot(Vector3f(0, 0, 1)));
	EXPECT_NEAR(angle * M_RAD_TO_DEG_F, 45.f, 1e-4f);
	EXPECT_FLOAT_EQ(body.length(), 1.f);
}

TEST(ControlMathTest, LimitTilt45degree)
{
	Vector3f body = Vector3f(1, 0, 0);
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 45.f);
	EXPECT_EQ(body, Vector3f(M_SQRT1_2_F, 0, M_SQRT1_2_F));

	body = Vector3f(0, 1, 0);
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 45.f);
	EXPECT_EQ(body, Vector3f(0, M_SQRT1_2_F, M_SQRT1_2_F));
}

TEST(ControlMathTest, LimitTilt10degree)
{
	Vector3f body = Vector3f(1, 1, .1f).normalized();
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 10.f);
	float angle = acosf(body.dot(Vector3f(0, 0, 1)));
	EXPECT_NEAR(angle * M_RAD_TO_DEG_F, 10.f, 1e-4f);
	EXPECT_FLOAT_EQ(body.length(), 1.f);
	EXPECT_FLOAT_EQ(body(0), body(1));

	body = Vector3f(1, 2, .2f);
	limitTilt(body, Vector3f(0, 0, 1), M_DEG_TO_RAD_F * 10.f);
	angle = acosf(body.dot(Vector3f(0, 0, 1)));
	EXPECT_NEAR(angle * M_RAD_TO_DEG_F, 10.f, 1e-4f);
	EXPECT_FLOAT_EQ(body.length(), 1.f);
	EXPECT_FLOAT_EQ(2.f * body(0), body(1));
}

TEST(ControlMathTest, ThrottleAttitudeMapping)
{
	/* expected: zero roll, zero pitch, zero yaw, full thr mag
	 * reasone: thrust pointing full upward */
	Vector3f thr{0.0f, 0.0f, -1.0f};
	float yaw = 0.0f;
	vehicle_attitude_setpoint_s att{};
	thrustToAttitude(att, thr, yaw);
	EXPECT_EQ(att.roll_body, 0);
	EXPECT_EQ(att.pitch_body, 0);
	EXPECT_EQ(att.yaw_body, 0);
	EXPECT_EQ(att.thrust_body[2], -1.f);

	/* expected: same as before but with 90 yaw
	 * reason: only yaw changed */
	yaw = M_PI_2_F;
	thrustToAttitude(att, thr, yaw);
	EXPECT_EQ(att.roll_body, 0);
	EXPECT_EQ(att.pitch_body, 0);
	EXPECT_EQ(att.yaw_body, M_PI_2_F);
	EXPECT_EQ(att.thrust_body[2], -1.f);

	/* expected: same as before but roll 180
	 * reason: thrust points straight down and order Euler
	 * order is: 1. roll, 2. pitch, 3. yaw */
	thr = Vector3f(0.0f, 0.0f, 1.0f);
	thrustToAttitude(att, thr, yaw);
	EXPECT_NEAR(abs(att.roll_body), M_PI_F, 1e4f);
	EXPECT_EQ(att.pitch_body, 0);
	EXPECT_EQ(att.yaw_body, M_PI_2_F);
	EXPECT_EQ(att.thrust_body[2], -1.f);
}

TEST(ControlMathTest, ConstrainXYPriorities)
{
	float max = 5.0f;
	// v0 already at max
	Vector2f v0(max, 0);
	Vector2f v1(v0(1), -v0(0));

	Vector2f v_r = constrainXY(v0, v1, max);
	EXPECT_EQ(v_r(0), max);
	EXPECT_GT(v_r(0), 0);
	EXPECT_EQ(v_r(1), 0);

	// v1 exceeds max but v0 is zero
	v0.zero();
	v_r = constrainXY(v0, v1, max);
	EXPECT_EQ(v_r(1), -max);
	EXPECT_LT(v_r(1), 0);
	EXPECT_EQ(v_r(0), 0);

	v0 = Vector2f(0.5f, 0.5f);
	v1 = Vector2f(0.5f, -0.5f);
	v_r = constrainXY(v0, v1, max);
	float diff = Vector2f(v_r - (v0 + v1)).length();
	EXPECT_EQ(diff, 0);

	// v0 and v1 exceed max and are perpendicular
	v0 = Vector2f(4.0f, 0.0f);
	v1 = Vector2f(0.0f, -4.0f);
	v_r = constrainXY(v0, v1, max);
	EXPECT_EQ(v_r(0), v0(0));
	EXPECT_GT(v_r(0), 0);
	float remaining = sqrtf(max * max - (v0(0) * v0(0)));
	EXPECT_EQ(v_r(1), -remaining);
}

TEST(ControlMathTest, CrossSphereLine)
{
	/* Testing 9 positions (+) around waypoints (o):
	 *
	 * Far             +              +              +
	 *
	 * Near            +              +              +
	 * On trajectory --+----o---------+---------o----+--
	 *                    prev                curr
	 *
	 * Expected targets (1, 2, 3):
	 * Far             +              +              +
	 *
	 *
	 * On trajectory -------1---------2---------3-------
	 *
	 *
	 * Near            +              +              +
	 * On trajectory -------o---1---------2-----3-------
	 *
	 *
	 * On trajectory --+----o----1----+--------2/3---+-- */
	Vector3f prev = Vector3f(0.0f, 0.0f, 0.0f);
	Vector3f curr = Vector3f(0.0f, 0.0f, 2.0f);
	Vector3f res;
	bool retval = false;

	// on line, near, before previous waypoint
	retval = cross_sphere_line(Vector3f(0.0f, 0.0f, -0.5f), 1.0f, prev, curr, res);
	EXPECT_TRUE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 0.5f));

	// on line, near, before target waypoint
	retval = cross_sphere_line(Vector3f(0.0f, 0.0f, 1.0f), 1.0f, prev, curr, res);
	EXPECT_TRUE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 2.f));

	// on line, near, after target waypoint
	retval = cross_sphere_line(Vector3f(0.0f, 0.0f, 2.5f), 1.0f, prev, curr, res);
	EXPECT_TRUE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 2.f));

	// near, before previous waypoint
	retval = cross_sphere_line(Vector3f(0.0f, 0.5f, -0.5f), 1.0f, prev, curr, res);
	EXPECT_TRUE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 0.366025388f));

	// near, before target waypoint
	retval = cross_sphere_line(Vector3f(0.0f, 0.5f, 1.0f), 1.0f, prev, curr, res);
	EXPECT_TRUE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 1.866025448f));

	// near, after target waypoint
	retval = ControlMath::cross_sphere_line(matrix::Vector3f(0.0f, 0.5f, 2.5f), 1.0f, prev, curr, res);
	EXPECT_TRUE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 2.f));

	// far, before previous waypoint
	retval = ControlMath::cross_sphere_line(matrix::Vector3f(0.0f, 2.0f, -0.5f), 1.0f, prev, curr, res);
	EXPECT_FALSE(retval);
	EXPECT_EQ(res, Vector3f());

	// far, before target waypoint
	retval = ControlMath::cross_sphere_line(matrix::Vector3f(0.0f, 2.0f, 1.0f), 1.0f, prev, curr, res);
	EXPECT_FALSE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 1.f));

	// far, after target waypoint
	retval = ControlMath::cross_sphere_line(matrix::Vector3f(0.0f, 2.0f, 2.5f), 1.0f, prev, curr, res);
	EXPECT_FALSE(retval);
	EXPECT_EQ(res, Vector3f(0.f, 0.f, 2.f));
}
