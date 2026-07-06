/*
 * Copyright (C) 2026  Really Slick Screensavers Contributors
 *
 * This file is part of rsUtility.
 *
 * rsUtility is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * rsUtility is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CATMULLROM_H
#define CATMULLROM_H

#include <cmath>

// Catmull-Rom spline interpolation between p1 and p2.
// p0 and p3 are neighboring control points for tangent calculation.
// t in [0,1] interpolates between p1 (t=0) and p2 (t=1).
// Values outside [0,1] extrapolate.
inline float catmullRom(float p0, float p1, float p2, float p3, float t){
	// Coefficients for cubic in Horner form: ((a * t + b) * t + c) * t + d
	const float a = -0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3;
	const float b = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
	const float c = -0.5f * p0 + 0.5f * p2;
	const float d = p1;
	return ((a * t + b) * t + c) * t + d;
}

// Ensure pt is at least minDist away from prev.
// If distance is effectively zero (distSq < (1e-8f)^2), uses fallback
// direction (minDist, 0, 0) relative to prev. If distance is positive but less
// than minDist, scales the existing offset to have length minDist.
inline void ensureMinDist(float* pt, const float* prev, float minDist){
	if(pt == nullptr || prev == nullptr || minDist <= 0.0f || !std::isfinite(minDist))
		return;

	float dx = pt[0] - prev[0];
	float dy = pt[1] - prev[1];
	float dz = pt[2] - prev[2];
	const float distSq = dx * dx + dy * dy + dz * dz;
	if(!std::isfinite(distSq))
		return;

	const float minDistSq = minDist * minDist;
	if(distSq >= minDistSq)
		return;

	const float kZeroOffsetDist = 1e-8f;
	const float kZeroOffsetDistSq = kZeroOffsetDist * kZeroOffsetDist;
	if(distSq < kZeroOffsetDistSq){
		// Zero (or near-zero) offset - use fallback direction
		pt[0] = prev[0] + minDist;
		pt[1] = prev[1];
		pt[2] = prev[2];
	} else {
		// Scale offset to minDist
		const float dist = std::sqrt(distSq);
		const float scale = minDist / dist;
		pt[0] = prev[0] + dx * scale;
		pt[1] = prev[1] + dy * scale;
		pt[2] = prev[2] + dz * scale;
	}
}

#endif // CATMULLROM_H
