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

#include "material/material_shiny_diffuse.h"
#include "common/param.h"
#include "sampler/sample.h"
#include "shader/shader_node.h"
#include "geometry/surface.h"
#include "common/logger.h"
#include "render/render_data.h"
#include <cstring>

BEGIN_YAFARAY

ShinyDiffuseMaterial::ShinyDiffuseMaterial(const Rgb &diffuse_color, const Rgb &mirror_color, float diffuse_strength, float transparency_strength, float translucency_strength, float mirror_strength, float emit_strength, float transmit_filter_strength, Visibility visibility):
		diffuse_color_(diffuse_color), mirror_color_(mirror_color),
		mirror_strength_(mirror_strength), transparency_strength_(transparency_strength), translucency_strength_(translucency_strength), diffuse_strength_(diffuse_strength), transmit_filter_strength_(transmit_filter_strength)
{
	visibility_ = visibility;
	emit_color_ = emit_strength * diffuse_color;
	emit_strength_ = emit_strength;
	bsdf_flags_ = BsdfFlags::None;
	if(emit_strength_ > 0.f) bsdf_flags_ |= BsdfFlags::Emit;
	visibility_ = visibility;
}

ShinyDiffuseMaterial::~ShinyDiffuseMaterial()
{
	// Empty
}

/*! ATTENTION! You *MUST* call this function before using the material, no matter
    if you want to use shaderNodes or not!
*/
void ShinyDiffuseMaterial::config()
{
	n_bsdf_ = 0;
	vi_nodes_[0] = vi_nodes_[1] = vi_nodes_[2] = vi_nodes_[3] = false;
	vd_nodes_[0] = vd_nodes_[1] = vd_nodes_[2] = vd_nodes_[3] = false;
	float acc = 1.f;
	if(mirror_strength_ > 0.00001f || mirror_shader_)
	{
		is_mirror_ = true;
		if(mirror_shader_) { if(mirror_shader_->isViewDependant())vd_nodes_[0] = true; else vi_nodes_[0] = true; }
		else if(!m_has_fresnel_effect_) acc = 1.f - mirror_strength_;
		bsdf_flags_ |= BsdfFlags::Specular | BsdfFlags::Reflect;
		c_flags_[n_bsdf_] = BsdfFlags::Specular | BsdfFlags::Reflect;
		c_index_[n_bsdf_] = 0;
		++n_bsdf_;
	}
	if(transparency_strength_ * acc > 0.00001f || transparency_shader_)
	{
		m_is_transparent_ = true;
		if(transparency_shader_) { if(transparency_shader_->isViewDependant())vd_nodes_[1] = true; else vi_nodes_[1] = true; }
		else acc *= 1.f - transparency_strength_;
		bsdf_flags_ |= BsdfFlags::Transmit | BsdfFlags::Filter;
		c_flags_[n_bsdf_] = BsdfFlags::Transmit | BsdfFlags::Filter;
		c_index_[n_bsdf_] = 1;
		++n_bsdf_;
	}
	if(translucency_strength_ * acc > 0.00001f || translucency_shader_)
	{
		m_is_translucent_ = true;
		if(translucency_shader_) { if(translucency_shader_->isViewDependant())vd_nodes_[2] = true; else vi_nodes_[2] = true; }
		else acc *= 1.f - transparency_strength_;
		bsdf_flags_ |= BsdfFlags::Diffuse | BsdfFlags::Transmit;
		c_flags_[n_bsdf_] = BsdfFlags::Diffuse | BsdfFlags::Transmit;
		c_index_[n_bsdf_] = 2;
		++n_bsdf_;
	}
	if(diffuse_strength_ * acc > 0.00001f)
	{
		is_diffuse_ = true;
		if(diffuse_shader_) { if(diffuse_shader_->isViewDependant())vd_nodes_[3] = true; else vi_nodes_[3] = true; }
		bsdf_flags_ |= BsdfFlags::Diffuse | BsdfFlags::Reflect;
		c_flags_[n_bsdf_] = BsdfFlags::Diffuse | BsdfFlags::Reflect;
		c_index_[n_bsdf_] = 3;
		++n_bsdf_;
	}
	req_mem_ = req_node_mem_ + sizeof(SdDat);
}

// component should be initialized with mMirrorStrength, mTransparencyStrength, mTranslucencyStrength, mDiffuseStrength
// since values for which useNode is false do not get touched so it can be applied
// twice, for view-independent (initBSDF) and view-dependent (sample/eval) nodes

int ShinyDiffuseMaterial::getComponents(const bool *use_node, NodeStack &stack, float *component) const
{
	if(is_mirror_)
	{
		component[0] = use_node[0] ? mirror_shader_->getScalar(stack) : mirror_strength_;
	}
	if(m_is_transparent_)
	{
		component[1] = use_node[1] ? transparency_shader_->getScalar(stack) : transparency_strength_;
	}
	if(m_is_translucent_)
	{
		component[2] = use_node[2] ? translucency_shader_->getScalar(stack) : translucency_strength_;
	}
	if(is_diffuse_)
	{
		component[3] = diffuse_strength_;
	}
	return 0;
}

inline void ShinyDiffuseMaterial::getFresnel(const Vec3 &wo, const Vec3 &n, float &kr, float &current_ior_squared) const
{
	if(m_has_fresnel_effect_)
	{
		Vec3 N;

		if((wo * n) < 0.f)
		{
			N = -n;
		}
		else
		{
			N = n;
		}

		float c = wo * N;
		float g = current_ior_squared + c * c - 1.f;
		if(g < 0.f) g = 0.f;
		else g = math::sqrt(g);
		float aux = c * (g + c);

		kr = ((0.5f * (g - c) * (g - c)) / ((g + c) * (g + c))) *
			 (1.f + ((aux - 1) * (aux - 1)) / ((aux + 1) * (aux + 1)));
	}
	else
	{
		kr = 1.f;
	}
}

// calculate the absolute value of scattering components from the "normalized"
// fractions which are between 0 (no scattering) and 1 (scatter all remaining light)
// Kr is an optional reflection multiplier (e.g. from Fresnel)
static inline void accumulate__(const float *component, float *accum, float kr)
{
	accum[0] = component[0] * kr;
	float acc = 1.f - accum[0];
	accum[1] = component[1] * acc;
	acc *= 1.f - component[1];
	accum[2] = component[2] * acc;
	acc *= 1.f - component[2];
	accum[3] = component[3] * acc;
}

void ShinyDiffuseMaterial::initBsdf(const RenderData &render_data, SurfacePoint &sp, BsdfFlags &bsdf_types) const
{
	SdDat *dat = (SdDat *)render_data.arena_;
	memset(dat, 0, 8 * sizeof(float));
	dat->node_stack_ = (char *)render_data.arena_ + sizeof(SdDat);
	//create our "stack" to save node results
	NodeStack stack(dat->node_stack_);

	//bump mapping (extremely experimental)
	if(bump_shader_)
	{
		evalBump(stack, render_data, sp, bump_shader_);
	}

	//eval viewindependent nodes
	auto end = all_viewindep_.end();
	for(auto iter = all_viewindep_.begin(); iter != end; ++iter)(*iter)->eval(stack, render_data, sp);
	bsdf_types = bsdf_flags_;

	getComponents(vi_nodes_, stack, dat->component_);
}

/** Initialize Oren Nayar reflectance.
 *  Initialize Oren Nayar A and B coefficient.
 *  @param  sigma Roughness of the surface
 */
void ShinyDiffuseMaterial::initOrenNayar(double sigma)
{
	double sigma_squared = sigma * sigma;
	oren_nayar_a_ = 1.0 - 0.5 * (sigma_squared / (sigma_squared + 0.33));
	oren_nayar_b_ = 0.45 * sigma_squared / (sigma_squared + 0.09);
	use_oren_nayar_ = true;
}

/** Calculate Oren Nayar reflectance.
 *  Calculate Oren Nayar reflectance for a given reflection.
 *  @param  wi Reflected ray direction
 *  @param  wo Incident ray direction
 *  @param  n  Surface normal
 *  @note   http://en.wikipedia.org/wiki/Oren-Nayar_reflectance_model
 */
float ShinyDiffuseMaterial::orenNayar(const Vec3 &wi, const Vec3 &wo, const Vec3 &n, bool use_texture_sigma, double texture_sigma) const
{
	float cos_ti = std::max(-1.f, std::min(1.f, n * wi));
	float cos_to = std::max(-1.f, std::min(1.f, n * wo));
	float maxcos_f = 0.f;

	if(cos_ti < 0.9999f && cos_to < 0.9999f)
	{
		Vec3 v_1 = (wi - n * cos_ti).normalize();
		Vec3 v_2 = (wo - n * cos_to).normalize();
		maxcos_f = std::max(0.f, v_1 * v_2);
	}

	float sin_alpha, tan_beta;

	if(cos_to >= cos_ti)
	{
		sin_alpha = math::sqrt(1.f - cos_ti * cos_ti);
		tan_beta = math::sqrt(1.f - cos_to * cos_to) / ((cos_to == 0.f) ? 1e-8f : cos_to); // white (black on windows) dots fix for oren-nayar, could happen with bad normals
	}
	else
	{
		sin_alpha = math::sqrt(1.f - cos_to * cos_to);
		tan_beta = math::sqrt(1.f - cos_ti * cos_ti) / ((cos_ti == 0.f) ? 1e-8f : cos_ti); // white (black on windows) dots fix for oren-nayar, could happen with bad normals
	}

	if(use_texture_sigma)
	{
		double sigma_squared = texture_sigma * texture_sigma;
		double m_oren_nayar_texture_a = 1.0 - 0.5 * (sigma_squared / (sigma_squared + 0.33));
		double m_oren_nayar_texture_b = 0.45 * sigma_squared / (sigma_squared + 0.09);
		return std::min(1.f, std::max(0.f, (float)(m_oren_nayar_texture_a + m_oren_nayar_texture_b * maxcos_f * sin_alpha * tan_beta)));
	}
	else
	{
		return std::min(1.f, std::max(0.f, (float)(oren_nayar_a_ + oren_nayar_b_ * maxcos_f * sin_alpha * tan_beta)));
	}
}


Rgb ShinyDiffuseMaterial::eval(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wl, const BsdfFlags &bsdfs, bool force_eval) const
{
	float cos_ng_wo = sp.ng_ * wo;
	float cos_ng_wl = sp.ng_ * wl;
	// face forward:
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	if(!bsdfs.hasAny(bsdf_flags_ & BsdfFlags::Diffuse)) return Rgb(0.f);

	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	float kr;

	float cur_ior_squared;
	if(ior_s_)
	{
		cur_ior_squared = ior_ + ior_s_->getScalar(stack);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = m_ior_squared_;

	getFresnel(wo, n, kr, cur_ior_squared);
	float m_t = (1.f - kr * dat->component_[0]) * (1.f - dat->component_[1]);

	bool transmit = (cos_ng_wo * cos_ng_wl) < 0.f;

	if(transmit) // light comes from opposite side of surface
	{
		if(m_is_translucent_) return dat->component_[2] * m_t * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_);
	}

	if(n * wl < 0.0 && !flat_material_) return Rgb(0.f);
	float m_d = m_t * (1.f - dat->component_[2]) * dat->component_[3];

	if(use_oren_nayar_)
	{
		double texture_sigma = (sigma_oren_shader_ ? sigma_oren_shader_->getScalar(stack) : 0.f);
		bool use_texture_sigma = (sigma_oren_shader_ ? true : false);
		if(use_oren_nayar_) m_d *= orenNayar(wo, wl, n, use_texture_sigma, texture_sigma);
	}

	if(diffuse_refl_shader_) m_d *= diffuse_refl_shader_->getScalar(stack);

	Rgb result = m_d * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_);

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(result, wire_frame_amount, sp);

	return result;
}

Rgb ShinyDiffuseMaterial::emit(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	Rgb result = (diffuse_shader_ ? diffuse_shader_->getColor(stack) * emit_strength_ : emit_color_);

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(result, wire_frame_amount, sp);

	return result;
}

Rgb ShinyDiffuseMaterial::sample(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, Vec3 &wi, Sample &s, float &w) const
{
	float accum_c[4];
	float cos_ng_wo = sp.ng_ * wo, cos_ng_wi, cos_n;
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);

	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	float kr;

	float cur_ior_squared;
	if(ior_s_)
	{
		cur_ior_squared = ior_ + ior_s_->getScalar(stack);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = m_ior_squared_;

	getFresnel(wo, n, kr, cur_ior_squared);
	accumulate__(dat->component_, accum_c, kr);

	float sum = 0.f, val[4], width[4];
	BsdfFlags choice[4];
	int n_match = 0, pick = -1;
	for(int i = 0; i < n_bsdf_; ++i)
	{
		if((s.flags_ & c_flags_[i]) == c_flags_[i])
		{
			width[n_match] = accum_c[c_index_[i]];
			sum += width[n_match];
			choice[n_match] = c_flags_[i];
			val[n_match] = sum;
			++n_match;
		}
	}
	if(!n_match || sum < 0.00001) { s.sampled_flags_ = BsdfFlags::None; s.pdf_ = 0.f; return Rgb(1.f); }
	float inv_sum = 1.f / sum;
	for(int i = 0; i < n_match; ++i)
	{
		val[i] *= inv_sum;
		width[i] *= inv_sum;
		if((s.s_1_ <= val[i]) && (pick < 0)) pick = i;
	}
	if(pick < 0) pick = n_match - 1;
	float s_1;
	if(pick > 0) s_1 = (s.s_1_ - val[pick - 1]) / width[pick];
	else s_1 = s.s_1_ / width[pick];

	Rgb scolor(0.f);
	switch(static_cast<unsigned int>(choice[pick]))
	{
		case(BsdfFlags::SpecularReflect):
			wi = Vec3::reflectDir(n, wo);
			s.pdf_ = width[pick];
			scolor = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : mirror_color_) * (accum_c[0]);
			if(s.reverse_)
			{
				s.pdf_back_ = s.pdf_;
				s.col_back_ = scolor / std::max(std::fabs(sp.n_ * wo), 1.0e-6f);
			}
			scolor *= 1.f / std::max(std::fabs(sp.n_ * wi), 1.0e-6f);
			break;
		case(BsdfFlags::SpecularTransmit):
			wi = -wo;
			scolor = accum_c[1] * (transmit_filter_strength_ * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_) + Rgb(1.f - transmit_filter_strength_));
			cos_n = std::fabs(wi * n);
			if(cos_n < 1e-6) s.pdf_ = 0.f;
			else s.pdf_ = width[pick];
			break;
		case(BsdfFlags::Translucency):
			wi = sample::cosHemisphere(-n, sp.nu_, sp.nv_, s_1, s.s_2_);
			cos_ng_wi = sp.ng_ * wi;
			if(cos_ng_wo * cos_ng_wi < 0) scolor = accum_c[2] * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_);
			s.pdf_ = std::fabs(wi * n) * width[pick]; break;
		case(BsdfFlags::DiffuseReflect):
		default:
			wi = sample::cosHemisphere(n, sp.nu_, sp.nv_, s_1, s.s_2_);
			cos_ng_wi = sp.ng_ * wi;
			if(cos_ng_wo * cos_ng_wi > 0) scolor = accum_c[3] * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_);

			if(use_oren_nayar_)
			{
				double texture_sigma = (sigma_oren_shader_ ? sigma_oren_shader_->getScalar(stack) : 0.f);
				bool use_texture_sigma = (sigma_oren_shader_ ? true : false);

				scolor *= orenNayar(wo, wi, n, use_texture_sigma, texture_sigma);
			}
			s.pdf_ = std::fabs(wi * n) * width[pick]; break;
	}
	s.sampled_flags_ = choice[pick];
	w = (std::fabs(wi * sp.n_)) / (s.pdf_ * 0.99f + 0.01f);

	const float alpha = getAlpha(render_data, sp, wo);
	w = w * (alpha) + 1.f * (1.f - alpha);

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(scolor, wire_frame_amount, sp);

	return scolor;
}

float ShinyDiffuseMaterial::pdf(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, const Vec3 &wi, const BsdfFlags &bsdfs) const
{
	if(!bsdfs.hasAny(BsdfFlags::Diffuse)) return 0.f;

	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	float pdf = 0.f;
	float accum_c[4];
	float cos_ng_wo = sp.ng_ * wo, cos_ng_wi;
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
	float kr;

	float cur_ior_squared;
	if(ior_s_)
	{
		cur_ior_squared = ior_ + ior_s_->getScalar(stack);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = m_ior_squared_;

	getFresnel(wo, n, kr, cur_ior_squared);

	accumulate__(dat->component_, accum_c, kr);
	float sum = 0.f, width;
	int n_match = 0;
	for(int i = 0; i < n_bsdf_; ++i)
	{
		if(bsdfs.hasAny(c_flags_[i]))
		{
			width = accum_c[c_index_[i]];
			sum += width;

			switch(static_cast<unsigned int>(c_flags_[i]))
			{
				case(BsdfFlags::Diffuse | BsdfFlags::Transmit):  // translucency (diffuse transmitt)
					cos_ng_wi = sp.ng_ * wi;
					if(cos_ng_wo * cos_ng_wi < 0) pdf += std::fabs(wi * n) * width;
					break;

				case(BsdfFlags::Diffuse | BsdfFlags::Reflect):  // lambertian
					cos_ng_wi = sp.ng_ * wi;
					pdf += std::fabs(wi * n) * width;
					break;
				default:
					break;
			}
			++n_match;
		}
	}
	if(!n_match || sum < 0.00001) return 0.f;
	return pdf / sum;
}


/** Perfect specular reflection.
 *  Calculate perfect specular reflection and refraction from the material for
 *  a given surface point \a sp and a given incident ray direction \a wo
 *  @param  render_data Render state
 *  @param  sp Surface point
 *  @param  wo Incident ray direction
 *  @param  do_reflect Boolean value which is true if you have a reflection, false otherwise
 *  @param  do_refract Boolean value which is true if you have a refraction, false otherwise
 *  @param  wi Array of two vectors to record reflected ray direction (wi[0]) and refracted ray direction (wi[1])
 *  @param  col Array of two colors to record reflected ray color (col[0]) and refracted ray color (col[1])
 */
void ShinyDiffuseMaterial::getSpecular(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo, bool &do_reflect, bool &do_refract, Vec3 *const wi, Rgb *const col) const
{
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	const bool backface = wo * sp.ng_ < 0.f;
	const Vec3 n  = backface ? -sp.n_ : sp.n_;
	const Vec3 ng = backface ? -sp.ng_ : sp.ng_;

	float kr;

	float cur_ior_squared;
	if(ior_s_)
	{
		cur_ior_squared = ior_ + ior_s_->getScalar(stack);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = m_ior_squared_;

	getFresnel(wo, n, kr, cur_ior_squared);

	if(m_is_transparent_)
	{
		do_refract = true;
		wi[1] = -wo;
		Rgb tcol = transmit_filter_strength_ * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_) + Rgb(1.f - transmit_filter_strength_);
		col[1] = (1.f - dat->component_[0] * kr) * dat->component_[1] * tcol;
	}
	else
	{
		do_refract = false;
	}

	if(is_mirror_)
	{
		do_reflect = true;
		//Y_WARNING << sp.N << " | " << N << YENDL;
		wi[0] = wo;
		wi[0].reflect(n);
		float cos_wi_ng = wi[0] * ng;
		if(cos_wi_ng < 0.01)
		{
			wi[0] += (0.01 - cos_wi_ng) * ng;
			wi[0].normalize();
		}
		col[0] = (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : mirror_color_) * (dat->component_[0] * kr);
	}
	else
	{
		do_reflect = false;
	}

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(col, wire_frame_amount, sp);
}

Rgb ShinyDiffuseMaterial::getTransparency(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	if(!m_is_transparent_) return Rgb(0.f);

	NodeStack stack(render_data.arena_);
	auto end = all_sorted_.end();
	for(auto iter = all_sorted_.begin(); iter != end; ++iter)(*iter)->eval(stack, render_data, sp);
	float accum = 1.f;
	float kr;
	Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);

	float cur_ior_squared;
	if(ior_s_)
	{
		cur_ior_squared = ior_ + ior_s_->getScalar(stack);
		cur_ior_squared *= cur_ior_squared;
	}
	else cur_ior_squared = m_ior_squared_;

	getFresnel(wo, n, kr, cur_ior_squared);

	if(is_mirror_)
	{
		accum = 1.f - kr * (mirror_shader_ ? mirror_shader_->getScalar(stack) : mirror_strength_);
	}
	if(m_is_transparent_) //uhm...should actually be true if this function gets called anyway...
	{
		accum *= transparency_shader_ ? transparency_shader_->getScalar(stack) * accum : transparency_strength_ * accum;
	}
	Rgb tcol = transmit_filter_strength_ * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_) + Rgb(1.f - transmit_filter_strength_);

	Rgb result = accum * tcol;

	float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
	applyWireFrame(result, wire_frame_amount, sp);

	return result;
}

float ShinyDiffuseMaterial::getAlpha(const RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	if(m_is_transparent_)
	{
		Vec3 n = SurfacePoint::normalFaceForward(sp.ng_, sp.n_, wo);
		float kr;

		float cur_ior_squared;
		if(ior_s_)
		{
			cur_ior_squared = ior_ + ior_s_->getScalar(stack);
			cur_ior_squared *= cur_ior_squared;
		}
		else cur_ior_squared = m_ior_squared_;

		getFresnel(wo, n, kr, cur_ior_squared);
		float refl = (1.f - dat->component_[0] * kr) * dat->component_[1];
		float result = 1.f - refl;

		float wire_frame_amount = (wireframe_shader_ ? wireframe_shader_->getScalar(stack) * wireframe_amount_ : wireframe_amount_);
		applyWireFrame(result, wire_frame_amount, sp);

		return result;
	}
	return 1.f;
}

Material *ShinyDiffuseMaterial::factory(ParamMap &params, std::list<ParamMap> &params_list, Scene &scene)
{
	/// Material Parameters
	Rgb diffuse_color = 1.f;
	Rgb mirror_color = 1.f;
	float diffuse_strength = 1.f;
	float transparency_strength = 0.f;
	float translucency_strength = 0.f;
	float mirror_strength = 0.f;
	float emit_strength = 0.f;
	bool has_fresnel_effect = false;
	std::string s_visibility = "normal";
	Visibility visibility = Material::Visibility::NormalVisible;
	bool receive_shadows = true;
	bool flat_material = false;
	float ior = 1.33f;
	double transmit_filter_strength = 1.0;
	int mat_pass_index = 0;
	int additionaldepth = 0;
	float transparentbias_factor = 0.f;
	bool transparentbias_multiply_raydepth = false;
	float samplingfactor = 1.f;
	float wire_frame_amount = 0.f;           //!< Wireframe shading amount
	float wire_frame_thickness = 0.01f;      //!< Wireframe thickness
	float wire_frame_exponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
	Rgb wire_frame_color = Rgb(1.f); //!< Wireframe shading color

	params.getParam("color", diffuse_color);
	params.getParam("mirror_color", mirror_color);
	params.getParam("transparency", transparency_strength);
	params.getParam("translucency", translucency_strength);
	params.getParam("diffuse_reflect", diffuse_strength);
	params.getParam("specular_reflect", mirror_strength);
	params.getParam("emit", emit_strength);
	params.getParam("IOR", ior);
	params.getParam("fresnel_effect", has_fresnel_effect);
	params.getParam("transmit_filter", transmit_filter_strength);

	params.getParam("receive_shadows",  receive_shadows);
	params.getParam("flat_material",  flat_material);
	params.getParam("visibility", s_visibility);
	params.getParam("mat_pass_index",   mat_pass_index);
	params.getParam("additionaldepth",   additionaldepth);
	params.getParam("transparentbias_factor",   transparentbias_factor);
	params.getParam("transparentbias_multiply_raydepth",   transparentbias_multiply_raydepth);
	params.getParam("samplingfactor",   samplingfactor);

	params.getParam("wireframe_amount", wire_frame_amount);
	params.getParam("wireframe_thickness", wire_frame_thickness);
	params.getParam("wireframe_exponent", wire_frame_exponent);
	params.getParam("wireframe_color", wire_frame_color);

	if(s_visibility == "normal") visibility = Material::Visibility::NormalVisible;
	else if(s_visibility == "no_shadows") visibility = Material::Visibility::VisibleNoShadows;
	else if(s_visibility == "shadow_only") visibility = Material::Visibility::InvisibleShadowsOnly;
	else if(s_visibility == "invisible") visibility = Material::Visibility::Invisible;
	else visibility = Material::Visibility::NormalVisible;

	// !!remember to put diffuse multiplier in material itself!
	ShinyDiffuseMaterial *mat = new ShinyDiffuseMaterial(diffuse_color, mirror_color, diffuse_strength, transparency_strength, translucency_strength, mirror_strength, emit_strength, transmit_filter_strength, visibility);

	mat->setMaterialIndex(mat_pass_index);
	mat->receive_shadows_ = receive_shadows;
	mat->flat_material_ = flat_material;
	mat->additional_depth_ = additionaldepth;
	mat->transparent_bias_factor_ = transparentbias_factor;
	mat->transparent_bias_multiply_ray_depth_ = transparentbias_multiply_raydepth;

	mat->wireframe_amount_ = wire_frame_amount;
	mat->wireframe_thickness_ = wire_frame_thickness;
	mat->wireframe_exponent_ = wire_frame_exponent;
	mat->wireframe_color_ = wire_frame_color;

	mat->setSamplingFactor(samplingfactor);

	if(has_fresnel_effect)
	{
		mat->ior_ = ior;
		mat->m_ior_squared_ = ior * ior;
		mat->m_has_fresnel_effect_ = true;
	}

	std::string name;
	if(params.getParam("diffuse_brdf", name))
	{
		if(name == "oren_nayar")
		{
			double sigma = 0.1;
			params.getParam("sigma", sigma);
			mat->initOrenNayar(sigma);
		}
	}

	/// Material Shader Nodes
	std::vector<ShaderNode *> roots;
	std::map<std::string, ShaderNode *> node_list;

	// prepare shader nodes list
	node_list["diffuse_shader"]      = nullptr;
	node_list["mirror_color_shader"] = nullptr;
	node_list["bump_shader"]         = nullptr;
	node_list["mirror_shader"]       = nullptr;
	node_list["transparency_shader"] = nullptr;
	node_list["translucency_shader"] = nullptr;
	node_list["sigma_oren_shader"]   = nullptr;
	node_list["diffuse_refl_shader"] = nullptr;
	node_list["IOR_shader"]          = nullptr;
	node_list["wireframe_shader"]    = nullptr;

	// load shader nodes:
	if(mat->loadNodes(params_list, scene))
	{
		mat->parseNodes(params, roots, node_list);
	}
	else Y_ERROR << "ShinyDiffuse: Loading shader nodes failed!" << YENDL;

	mat->diffuse_shader_      = node_list["diffuse_shader"];
	mat->mirror_color_shader_  = node_list["mirror_color_shader"];
	mat->bump_shader_         = node_list["bump_shader"];
	mat->mirror_shader_       = node_list["mirror_shader"];
	mat->transparency_shader_ = node_list["transparency_shader"];
	mat->translucency_shader_ = node_list["translucency_shader"];
	mat->sigma_oren_shader_    = node_list["sigma_oren_shader"];
	mat->diffuse_refl_shader_  = node_list["diffuse_refl_shader"];
	mat->ior_s_                = node_list["IOR_shader"];
	mat->wireframe_shader_    = node_list["wireframe_shader"];

	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);

		std::vector<ShaderNode *> color_nodes;

		if(mat->diffuse_shader_)      mat->getNodeList(mat->diffuse_shader_, color_nodes);
		if(mat->mirror_color_shader_)  mat->getNodeList(mat->mirror_color_shader_, color_nodes);
		if(mat->mirror_shader_)       mat->getNodeList(mat->mirror_shader_, color_nodes);
		if(mat->transparency_shader_) mat->getNodeList(mat->transparency_shader_, color_nodes);
		if(mat->translucency_shader_) mat->getNodeList(mat->translucency_shader_, color_nodes);
		if(mat->sigma_oren_shader_)    mat->getNodeList(mat->sigma_oren_shader_, color_nodes);
		if(mat->diffuse_refl_shader_)  mat->getNodeList(mat->diffuse_refl_shader_, color_nodes);
		if(mat->ior_s_)                mat->getNodeList(mat->ior_s_, color_nodes);
		if(mat->wireframe_shader_)    mat->getNodeList(mat->wireframe_shader_, color_nodes);

		mat->filterNodes(color_nodes, mat->all_viewdep_, ViewDep);
		mat->filterNodes(color_nodes, mat->all_viewindep_, ViewIndep);

		if(mat->bump_shader_)         mat->getNodeList(mat->bump_shader_, mat->bump_nodes_);
	}


	mat->config();

	//===!!!=== test <<< This test should go, is useless, DT
	/*if(params.getParam("name", name))
	{
	    if(name->substr(0, 6) == "MAsss_")
	    {
	        paraMap_t map;
	        map["type"] = std::string("sss");
	        map["absorption_col"] = Rgb(0.5f, 0.2f, 0.2f);
	        map["absorption_dist"] = 0.5f;
	        map["scatter_col"] = Rgb(0.9f);
	        mat->volI = scene.createVolumeH(*name, map);
	        mat->bsdfFlags |= BSDF_VOLUMETRIC;
	    }
	}*/
	//===!!!=== end of test

	return mat;
}

Rgb ShinyDiffuseMaterial::getDiffuseColor(const RenderData &render_data) const {
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	if(is_diffuse_) return (diffuse_refl_shader_ ? diffuse_refl_shader_->getScalar(stack) : diffuse_strength_) * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_);
	else return Rgb(0.f);
}

Rgb ShinyDiffuseMaterial::getGlossyColor(const RenderData &render_data) const {
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	if(is_mirror_) return (mirror_shader_ ? mirror_shader_->getScalar(stack) : mirror_strength_) * (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : mirror_color_);
	else return Rgb(0.f);
}

Rgb ShinyDiffuseMaterial::getTransColor(const RenderData &render_data) const {
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	if(m_is_transparent_) return (transparency_shader_ ? transparency_shader_->getScalar(stack) : transparency_strength_) * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_);
	else return Rgb(0.f);
}

Rgb ShinyDiffuseMaterial::getMirrorColor(const RenderData &render_data) const {
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	if(is_mirror_) return (mirror_shader_ ? mirror_shader_->getScalar(stack) : mirror_strength_) * (mirror_color_shader_ ? mirror_color_shader_->getColor(stack) : mirror_color_);
	else return Rgb(0.f);
}

Rgb ShinyDiffuseMaterial::getSubSurfaceColor(const RenderData &render_data) const {
	SdDat *dat = (SdDat *)render_data.arena_;
	NodeStack stack(dat->node_stack_);

	if(m_is_translucent_) return (translucency_shader_ ? translucency_shader_->getScalar(stack) : translucency_strength_) * (diffuse_shader_ ? diffuse_shader_->getColor(stack) : diffuse_color_);
	else return Rgb(0.f);
}

END_YAFARAY
