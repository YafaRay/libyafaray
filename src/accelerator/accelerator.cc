/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "accelerator/accelerator.h"
#include "accelerator/accelerator_kdtree.h"
#include "accelerator/accelerator_kdtree_multi_thread.h"
#include "accelerator/accelerator_simple_test.h"
#include "common/logger.h"
#include "common/param.h"
#include "geometry/surface.h"
#include "render/render_data.h"
#include "scene/yafaray/primitive_face.h"
#include "integrator/integrator.h"
#include "geometry/object.h"
#include "material/material.h"
#include "photon/photon.h"

BEGIN_YAFARAY

std::unique_ptr<Accelerator> Accelerator::factory(Logger &logger, const std::vector<const Primitive *> &primitives_list, ParamMap &params)
{
	if(logger.isDebug())
	{
		logger.logDebug("**Accelerator");
		params.logContents(logger);
	}
	std::string type;
	params.getParam("type", type);
	std::unique_ptr<Accelerator> accelerator;
	if(type == "yafaray-kdtree-original") accelerator = AcceleratorKdTree::factory(logger, primitives_list, params);
	else if(type == "yafaray-kdtree-multi-thread") accelerator = AcceleratorKdTreeMultiThread::factory(logger, primitives_list, params);
	else if(type == "yafaray-simpletest") accelerator = AcceleratorSimpleTest::factory(logger, primitives_list, params);

	if(accelerator) logger.logInfo("Accelerator type '", type, "' created.");
	else
	{
		logger.logWarning("Accelerator type '", type, "' could not be created, using standard single-thread KdTree instead.");
		accelerator = AcceleratorKdTree::factory(logger, primitives_list, params);
	}
	return accelerator;
}

bool Accelerator::intersect(const Ray &ray, SurfacePoint &sp) const
{
	const float t_max = (ray.tmax_ >= 0.f) ? ray.tmax_ : std::numeric_limits<float>::infinity();
	// intersect with tree:
	const AcceleratorIntersectData accelerator_intersect_data = intersect(ray, t_max);
	if(accelerator_intersect_data.hit_ && accelerator_intersect_data.hit_primitive_)
	{
		const Point3 hit_point = ray.from_ + accelerator_intersect_data.t_max_ * ray.dir_;
		sp = accelerator_intersect_data.hit_primitive_->getSurface(hit_point, accelerator_intersect_data);
		sp.hit_primitive_ = accelerator_intersect_data.hit_primitive_;
		sp.ray_ = nullptr;
		ray.tmax_ = accelerator_intersect_data.t_max_;
		return true;
	}
	return false;
}

bool Accelerator::intersect(const DiffRay &ray, SurfacePoint &sp) const
{
	if(!intersect(static_cast<const Ray&>(ray), sp)) return false;
	sp.ray_ = &ray;
	return true;
}

bool Accelerator::isShadowed(const RenderData &render_data, const Ray &ray, float &obj_index, float &mat_index, float shadow_bias) const
{
	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = render_data.time_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	const AcceleratorIntersectData accelerator_intersect_data = intersectS(sray, t_max, shadow_bias);
	if(accelerator_intersect_data.hit_)
	{
		if(accelerator_intersect_data.hit_primitive_)
		{
			if(accelerator_intersect_data.hit_primitive_->getObject()) obj_index = accelerator_intersect_data.hit_primitive_->getObject()->getAbsObjectIndex();    //Object index of the object casting the shadow
			if(accelerator_intersect_data.hit_primitive_->getMaterial()) mat_index = accelerator_intersect_data.hit_primitive_->getMaterial()->getAbsMaterialIndex();    //Material index of the object casting the shadow
		}
		return true;
	}
	return false;
}

bool Accelerator::isShadowed(RenderData &render_data, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index, float shadow_bias) const
{
	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	const float t_max = (ray.tmax_ >= 0.f) ? sray.tmax_ - 2 * sray.tmin_ : std::numeric_limits<float>::infinity();
	void *odat = render_data.arena_;
	alignas (16) unsigned char userdata[Integrator::getUserDataSize()];
	render_data.arena_ = static_cast<void *>(userdata);
	bool intersect = false;
	const AcceleratorTsIntersectData accelerator_intersect_data = intersectTs(render_data, sray, max_depth, t_max, shadow_bias);
	filt = accelerator_intersect_data.transparent_color_;
	if(accelerator_intersect_data.hit_)
	{
		intersect = true;
		if(accelerator_intersect_data.hit_primitive_)
		{
			if(accelerator_intersect_data.hit_primitive_->getObject()) obj_index = accelerator_intersect_data.hit_primitive_->getObject()->getAbsObjectIndex();    //Object index of the object casting the shadow
			if(accelerator_intersect_data.hit_primitive_->getMaterial()) mat_index = accelerator_intersect_data.hit_primitive_->getMaterial()->getAbsMaterialIndex();    //Material index of the object casting the shadow
		}
	}
	render_data.arena_ = odat;
	return intersect;
}

END_YAFARAY
