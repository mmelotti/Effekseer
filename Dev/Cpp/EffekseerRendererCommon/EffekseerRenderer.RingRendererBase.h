﻿
#ifndef	__EFFEKSEERRENDERER_RING_RENDERER_BASE_H__
#define	__EFFEKSEERRENDERER_RING_RENDERER_BASE_H__

//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include <Effekseer.h>
#include <Effekseer.Internal.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "EffekseerRenderer.CommonUtils.h"
#include "EffekseerRenderer.RenderStateBase.h"
#include "EffekseerRenderer.VertexBufferBase.h"
#include "EffekseerRenderer.IndexBufferBase.h"
#include "EffekseerRenderer.StandardRenderer.h"

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace EffekseerRenderer
{
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
typedef ::Effekseer::RingRenderer::NodeParameter efkRingNodeParam;
typedef ::Effekseer::RingRenderer::InstanceParameter efkRingInstanceParam;
typedef ::Effekseer::Vec3f efkVector3D;

template<typename RENDERER, typename VERTEX_NORMAL, typename VERTEX_DISTORTION>
class RingRendererBase
	: public ::Effekseer::RingRenderer
	, public ::Effekseer::AlignedAllocationPolicy<16>
{
protected:

	struct KeyValue
	{
		float Key;
		efkRingInstanceParam Value;
	};
	std::vector<KeyValue> instances_;

	RENDERER*						m_renderer;
	int32_t							m_ringBufferOffset;
	uint8_t*						m_ringBufferData;

	int32_t							m_spriteCount;
	int32_t							m_instanceCount;
	::Effekseer::Mat44f			m_singleRenderingMatrix;
	::Effekseer::RendererMaterialType materialType_ = ::Effekseer::RendererMaterialType::Default;

	int32_t vertexCount_ = 0;
	int32_t stride_ = 0;
	int32_t customData1Count_ = 0;
	int32_t customData2Count_ = 0;

public:

	RingRendererBase(RENDERER* renderer)
		: m_renderer(renderer)
		, m_ringBufferOffset(0)
		, m_ringBufferData(NULL)
		, m_spriteCount(0)
		, m_instanceCount(0)
	{
	}

	virtual ~RingRendererBase()
	{
	}

protected:

	void RenderingInstance(const efkRingInstanceParam& inst,
						   const efkRingNodeParam& param,
						   const StandardRendererState& state,
						   const ::Effekseer::Mat44f& camera)
	{
		if (state.MaterialPtr != nullptr && !state.MaterialPtr->IsSimpleVertex)
		{
			Rendering_Internal<DynamicVertex>(param, inst, nullptr, camera);
		}
		else if (param.BasicParameterPtr->MaterialType == Effekseer::RendererMaterialType::Lighting)
		{
			Rendering_Internal<LightingVertex>(param, inst, nullptr, camera);
		}
		else if (param.BasicParameterPtr->MaterialType == Effekseer::RendererMaterialType::BackDistortion)
		{
			Rendering_Internal<VERTEX_DISTORTION>(param, inst, nullptr, camera);
		}
		else
		{
			Rendering_Internal<VERTEX_NORMAL>(param, inst, nullptr, camera);
		}
	}

	void BeginRendering_(RENDERER* renderer, int32_t count, const efkRingNodeParam& param)
	{
		m_spriteCount = 0;
		int32_t singleVertexCount = param.VertexCount * 8;
		m_instanceCount = count;

		instances_.clear();

		if (param.DepthParameterPtr->ZSort != Effekseer::ZSortType::None)
		{
			instances_.reserve(count);
		}

		if (count == 1)
		{
			renderer->GetStandardRenderer()->ResetAndRenderingIfRequired();
		}
		
		EffekseerRenderer::StandardRendererState state;
		state.AlphaBlend = param.BasicParameterPtr->AlphaBlend;
		state.CullingType = ::Effekseer::CullingType::Double;
		state.DepthTest = param.ZTest;
		state.DepthWrite = param.ZWrite;
		state.TextureFilter1 = param.BasicParameterPtr->TextureFilter1;
		state.TextureWrap1 = param.BasicParameterPtr->TextureWrap1;
		state.TextureFilter2 = param.BasicParameterPtr->TextureFilter2;
		state.TextureWrap2 = param.BasicParameterPtr->TextureWrap2;
#ifdef __EFFEKSEER_BUILD_VERSION16__
		state.TextureFilter3 = param.BasicParameterPtr->TextureFilter3;
		state.TextureWrap3 = param.BasicParameterPtr->TextureWrap3;
		state.TextureFilter4 = param.BasicParameterPtr->TextureFilter4;
		state.TextureWrap4 = param.BasicParameterPtr->TextureWrap4;
		state.TextureFilter5 = param.BasicParameterPtr->TextureFilter5;
		state.TextureWrap5 = param.BasicParameterPtr->TextureWrap5;

		state.EnableInterpolation = param.BasicParameterPtr->EnableInterpolation;
		state.UVLoopType = param.BasicParameterPtr->UVLoopType;
		state.InterpolationType = param.BasicParameterPtr->InterpolationType;
		state.FlipbookDivideX = param.BasicParameterPtr->FlipbookDivideX;
		state.FlipbookDivideY = param.BasicParameterPtr->FlipbookDivideY;

		state.UVDistortionIntensity = param.BasicParameterPtr->UVDistortionIntensity;

		state.TextureBlendType = param.BasicParameterPtr->TextureBlendType;
#endif

		state.Distortion = param.BasicParameterPtr->MaterialType == Effekseer::RendererMaterialType::BackDistortion;
		state.DistortionIntensity = param.BasicParameterPtr->DistortionIntensity;
		state.MaterialType = param.BasicParameterPtr->MaterialType;

		state.CopyMaterialFromParameterToState(param.EffectPointer,
											   param.BasicParameterPtr->MaterialParameterPtr,
											   param.BasicParameterPtr->Texture1Index,
											   param.BasicParameterPtr->Texture2Index
#ifdef __EFFEKSEER_BUILD_VERSION16__
											   , param.BasicParameterPtr->Texture3Index
											   , param.BasicParameterPtr->Texture4Index
											   , param.BasicParameterPtr->Texture5Index
#endif
		);

		customData1Count_ = state.CustomData1Count;
		customData2Count_ = state.CustomData2Count;

		materialType_ = param.BasicParameterPtr->MaterialType;

		renderer->GetStandardRenderer()->UpdateStateAndRenderingIfRequired(state);
		renderer->GetStandardRenderer()->BeginRenderingAndRenderingIfRequired(count * singleVertexCount, stride_, (void*&)m_ringBufferData);

		vertexCount_ = count * singleVertexCount;
	}

	void Rendering_(const efkRingNodeParam& parameter, const efkRingInstanceParam& instanceParameter, void* userData, const ::Effekseer::Mat44f& camera)
	{
		if (parameter.DepthParameterPtr->ZSort == Effekseer::ZSortType::None || CanSingleRendering())
		{
			const auto& state = m_renderer->GetStandardRenderer()->GetState();

			RenderingInstance(instanceParameter, parameter, state, camera);
		}
		else
		{
			KeyValue kv;
			kv.Value = instanceParameter;
			instances_.push_back(kv);
		}
	}

	enum class VertexType
	{
		Normal,
		Distortion,
		Dynamic,
		Lighting,
	};

	VertexType GetVertexType(const VERTEX_NORMAL* v) { return VertexType::Normal; }

	VertexType GetVertexType(const VERTEX_DISTORTION* v) { return VertexType::Distortion; }

	VertexType GetVertexType(const DynamicVertex* v) { return VertexType::Dynamic; }

	VertexType GetVertexType(const LightingVertex* v) { return VertexType::Lighting; }

	bool CanSingleRendering()
	{ 
		return m_instanceCount <= 1 && materialType_ == ::Effekseer::RendererMaterialType::Default;
	}

	template<typename VERTEX>
	void Rendering_Internal( const efkRingNodeParam& parameter, const efkRingInstanceParam& instanceParameter, void* userData, const ::Effekseer::Mat44f& camera )
	{
		::Effekseer::Mat43f mat43;

		if (parameter.Billboard == ::Effekseer::BillboardType::Billboard ||
			parameter.Billboard == ::Effekseer::BillboardType::RotatedBillboard ||
			parameter.Billboard == ::Effekseer::BillboardType::YAxisFixed)
		{
			Effekseer::Vec3f s;
			Effekseer::Vec3f R;
			Effekseer::Vec3f F;

			CalcBillboard(parameter.Billboard, mat43, s, R, F, instanceParameter.SRTMatrix43, m_renderer->GetCameraFrontDirection());

			ApplyDepthParameters(mat43,
								 m_renderer->GetCameraFrontDirection(),
								 m_renderer->GetCameraPosition(),
								 s,
								 parameter.DepthParameterPtr,
								 parameter.IsRightHand);

			if (CanSingleRendering())
			{
				mat43 = ::Effekseer::Mat43f::Scaling(s) * mat43;
			}
			else
			{
				mat43 = ::Effekseer::Mat43f::Scaling(s) * mat43;
			}
		}
		else if (parameter.Billboard == ::Effekseer::BillboardType::Fixed)
		{
			mat43 = instanceParameter.SRTMatrix43;

			ApplyDepthParameters(mat43,
								 m_renderer->GetCameraFrontDirection(),
								 m_renderer->GetCameraPosition(),
								 parameter.DepthParameterPtr,
								 parameter.IsRightHand);
		}

		int32_t singleVertexCount = parameter.VertexCount * 8;
		//Vertex* verteies = (Vertex*)m_renderer->GetVertexBuffer()->GetBufferDirect( sizeof(Vertex) * vertexCount );
		
		StrideView<VERTEX> verteies(m_ringBufferData, stride_, singleVertexCount);
		auto vertexType = GetVertexType((VERTEX*)m_ringBufferData);

		const float circleAngleDegree = (instanceParameter.ViewingAngleEnd - instanceParameter.ViewingAngleStart);
		const float stepAngleDegree = circleAngleDegree / (parameter.VertexCount);
		const float stepAngle = (stepAngleDegree) / 180.0f * 3.141592f;
		const float beginAngle = (instanceParameter.ViewingAngleStart + 90) / 180.0f * 3.141592f;
		
		const float outerRadius = instanceParameter.OuterLocation.GetX();
		const float innerRadius = instanceParameter.InnerLocation.GetX();
		const float centerRadius = innerRadius + (outerRadius - innerRadius) * instanceParameter.CenterRatio;

		const float outerHeight = instanceParameter.OuterLocation.GetY();
		const float innerHeight = instanceParameter.InnerLocation.GetY();
		const float centerHeight = innerHeight + (outerHeight - innerHeight) * instanceParameter.CenterRatio;
		
		::Effekseer::Color outerColor = instanceParameter.OuterColor;
		::Effekseer::Color innerColor = instanceParameter.InnerColor;
		::Effekseer::Color centerColor = instanceParameter.CenterColor;
		::Effekseer::Color outerColorNext = instanceParameter.OuterColor;
		::Effekseer::Color innerColorNext = instanceParameter.InnerColor;
		::Effekseer::Color centerColorNext = instanceParameter.CenterColor;

		if (parameter.StartingFade > 0)
		{
			outerColor.A = 0;
			innerColor.A = 0;
			centerColor.A = 0;
		}

		const float stepC = cosf(stepAngle);
		const float stepS = sinf(stepAngle);
		float cos_ = cosf(beginAngle);
		float sin_ = sinf(beginAngle);
		::Effekseer::Vec3f outerCurrent( cos_ * outerRadius, sin_ * outerRadius, outerHeight );
		::Effekseer::Vec3f innerCurrent( cos_ * innerRadius, sin_ * innerRadius, innerHeight );
		::Effekseer::Vec3f centerCurrent( cos_ * centerRadius, sin_ * centerRadius, centerHeight );

		float uv0Current = instanceParameter.UV.X;
		const float uv0Step = instanceParameter.UV.Width / parameter.VertexCount;
		const float uv0v1 = instanceParameter.UV.Y;
		const float uv0v2 = uv0v1 + instanceParameter.UV.Height * 0.5f;
		const float uv0v3 = uv0v1 + instanceParameter.UV.Height;
		float uv0texNext = 0.0f;
		
		float uv1Current = 0.0f;
		const float uv1Step = 1.0f / parameter.VertexCount;
		const float uv1v1 = 0.0f;
		const float uv1v2 = uv1v1 + 0.5f;
		const float uv1v3 = uv1v1 + 1.0f;
		float uv1texNext = 0.0f;

#ifdef __EFFEKSEER_BUILD_VERSION16__
		const int32_t advancedUVNum = 3;

		float advancedUVCurrent[advancedUVNum] = 
		{ 
			instanceParameter.AlphaUV.X, 
			instanceParameter.UVDistortionUV.X 
		};
		const float advancedUVStep[advancedUVNum] = 
		{ 
			instanceParameter.AlphaUV.Width / parameter.VertexCount, 
			instanceParameter.UVDistortionUV.Width / parameter.VertexCount 
		};
		const float advancedUVv1[advancedUVNum] = 
		{ 
			instanceParameter.AlphaUV.Y, 
			instanceParameter.UVDistortionUV.Y 
		};
		const float advancedUVv2[advancedUVNum] =
		{
			advancedUVv1[0] + instanceParameter.AlphaUV.Height * 0.5f,
			advancedUVv1[1] + instanceParameter.UVDistortionUV.Height * 0.5f
		};
		const float advancedUVv3[advancedUVNum] =
		{
			advancedUVv1[0] + instanceParameter.AlphaUV.Height,
			advancedUVv1[1] + instanceParameter.UVDistortionUV.Height,
		};
		float advancedUVtexNext[advancedUVNum] =
		{
			0.0f
		};
#endif

		::Effekseer::Vec3f outerNext, innerNext, centerNext;

		float currentAngleDegree = 0;
		float fadeStartAngle = parameter.StartingFade;
		float fadeEndingAngle = parameter.EndingFade;

		for( int i = 0; i < singleVertexCount; i += 8 )
		{
			float old_c = cos_;
			float old_s = sin_;

			float t;
			t = cos_ * stepC - sin_ * stepS;
			sin_ = sin_ * stepC + cos_ * stepS;
			cos_ = t;

			outerNext = ::Effekseer::Vec3f{
				cos_ * outerRadius,
				sin_ * outerRadius,
				outerHeight};
			innerNext = ::Effekseer::Vec3f{
				cos_ * innerRadius,
				sin_ * innerRadius,
				innerHeight};
			centerNext = ::Effekseer::Vec3f{
				cos_ * centerRadius,
				sin_ * centerRadius,
				centerHeight};

			currentAngleDegree += stepAngleDegree;

			// for floating decimal point error
			currentAngleDegree = Effekseer::Min(currentAngleDegree, circleAngleDegree);
			float alpha = 1.0f;
			if (currentAngleDegree < fadeStartAngle)
			{
				alpha = currentAngleDegree / fadeStartAngle;
			}
			else if (currentAngleDegree > circleAngleDegree - fadeEndingAngle)
			{
				alpha = 1.0f - (currentAngleDegree - (circleAngleDegree - fadeEndingAngle)) / fadeEndingAngle;
			}

			outerColorNext = instanceParameter.OuterColor;
			innerColorNext = instanceParameter.InnerColor;
			centerColorNext = instanceParameter.CenterColor;

			if (alpha != 1.0f)
			{
				outerColorNext.A = static_cast<uint8_t>(outerColorNext.A * alpha);
				innerColorNext.A = static_cast<uint8_t>(innerColorNext.A * alpha);
				centerColorNext.A = static_cast<uint8_t>(centerColorNext.A * alpha);
			}

			uv0texNext = uv0Current + uv0Step;

			StrideView<VERTEX> v(&verteies[i], stride_, 8);
			v[0].Pos = ToStruct(outerCurrent);
			v[0].SetColor( outerColor );
			v[0].UV[0] = uv0Current;
			v[0].UV[1] = uv0v1;

			v[1].Pos = ToStruct(centerCurrent);
			v[1].SetColor( centerColor );
			v[1].UV[0] = uv0Current;
			v[1].UV[1] = uv0v2;

			v[2].Pos = ToStruct(outerNext);
			v[2].SetColor( outerColorNext );
			v[2].UV[0] = uv0texNext;
			v[2].UV[1] = uv0v1;
			
			v[3].Pos = ToStruct(centerNext);
			v[3].SetColor( centerColorNext );
			v[3].UV[0] = uv0texNext;
			v[3].UV[1] = uv0v2;

			v[4] = v[1];

			v[5].Pos = ToStruct(innerCurrent);
			v[5].SetColor( innerColor );
			v[5].UV[0] = uv0Current;
			v[5].UV[1] = uv0v3;

			v[6] = v[3];

			v[7].Pos = ToStruct(innerNext);
			v[7].SetColor( innerColorNext );
			v[7].UV[0] = uv0texNext;
			v[7].UV[1] = uv0v3;

#ifdef __EFFEKSEER_BUILD_VERSION16__
			for (int32_t uvi = 0; uvi < advancedUVNum; uvi++)
			{
				advancedUVtexNext[uvi] = advancedUVCurrent[uvi] + advancedUVStep[uvi];
			}

			v[0].SetAlphaUV(advancedUVCurrent[0], 0);
			v[0].SetAlphaUV(advancedUVv1[0], 1);			
			v[0].SetUVDistortionUV(advancedUVCurrent[1], 0);
			v[0].SetUVDistortionUV(advancedUVv1[1], 1);
			v[0].SetBlendUV(advancedUVCurrent[2], 0);
			v[0].SetBlendUV(advancedUVv1[2], 1);

			v[1].SetAlphaUV(advancedUVCurrent[0], 0);
			v[1].SetAlphaUV(advancedUVv2[0], 1);
			v[1].SetUVDistortionUV(advancedUVCurrent[1], 0);
			v[1].SetUVDistortionUV(advancedUVv2[1], 1);
			v[1].SetBlendUV(advancedUVCurrent[2], 0);
			v[1].SetBlendUV(advancedUVv2[2], 1);

			v[2].SetAlphaUV(advancedUVtexNext[0], 0);
			v[2].SetAlphaUV(advancedUVv1[0], 1);
			v[2].SetUVDistortionUV(advancedUVtexNext[1], 0);
			v[2].SetUVDistortionUV(advancedUVv1[1], 1);
			v[2].SetBlendUV(advancedUVtexNext[2], 0);
			v[2].SetBlendUV(advancedUVv1[2], 1);

			v[3].SetAlphaUV(advancedUVtexNext[0], 0);
			v[3].SetAlphaUV(advancedUVv2[0], 1);
			v[3].SetUVDistortionUV(advancedUVtexNext[1], 0);
			v[3].SetUVDistortionUV(advancedUVv2[1], 1);
			v[3].SetBlendUV(advancedUVtexNext[2], 0);
			v[3].SetBlendUV(advancedUVv2[2], 1);

			v[4] = v[1];

			v[5].SetAlphaUV(advancedUVCurrent[0], 0);
			v[5].SetAlphaUV(advancedUVv3[0], 1);
			v[5].SetUVDistortionUV(advancedUVCurrent[1], 0);
			v[5].SetUVDistortionUV(advancedUVv3[1], 1);
			v[5].SetBlendUV(advancedUVCurrent[2], 0);
			v[5].SetBlendUV(advancedUVv3[2], 1);

			v[6] = v[3];

			v[7].SetAlphaUV(advancedUVtexNext[0], 0);
			v[7].SetAlphaUV(advancedUVv3[0], 1);
			v[7].SetUVDistortionUV(advancedUVtexNext[1], 0);
			v[7].SetUVDistortionUV(advancedUVv3[1], 1);
			v[7].SetBlendUV(advancedUVtexNext[2], 0);
			v[7].SetBlendUV(advancedUVv3[2], 1);

			for (int32_t vi = 0; vi < 8; vi++)
			{
				v[vi].SetFlipbookIndexAndNextRate(instanceParameter.FlipbookIndexAndNextRate);
				v[vi].SetAlphaThreshold(instanceParameter.AlphaThreshold);
			}
#endif

			// distortion
			if (vertexType == VertexType::Distortion)
			{
				StrideView<VERTEX_DISTORTION> vs(&verteies[i], stride_, 8);
				auto binormalCurrent = v[5].Pos - v[0].Pos;
				auto binormalNext = v[7].Pos - v[2].Pos;

				// return back
				float t_b;
				t_b = old_c * (stepC) - old_s * (-stepS);
				auto s_b = old_s * (stepC) + old_c * (-stepS);
				auto c_b = t_b;

				::Effekseer::Vec3f outerBefore(
					c_b * outerRadius,
					s_b * outerRadius,
					outerHeight);

				// next
				auto t_n = cos_ * stepC - sin_ * stepS;
				auto s_n = sin_ * stepC + cos_ * stepS;
				auto c_n = t_n;

				::Effekseer::Vec3f outerNN(
					c_n * outerRadius,
					s_n * outerRadius,
					outerHeight);

				::Effekseer::Vec3f tangent0 = (outerCurrent - outerBefore).Normalize();
				::Effekseer::Vec3f tangent1 = (outerNext - outerCurrent).Normalize();
				::Effekseer::Vec3f tangent2 = (outerNN - outerNext).Normalize();

				auto tangentCurrent = (tangent0 + tangent1) / 2.0f;
				auto tangentNext = (tangent1 + tangent2) / 2.0f;

				vs[0].Tangent = ToStruct(tangentCurrent);
				vs[0].Binormal = ToStruct(binormalCurrent);
				vs[1].Tangent = ToStruct(tangentCurrent);
				vs[1].Binormal = ToStruct(binormalCurrent);
				vs[2].Tangent = ToStruct(tangentNext);
				vs[2].Binormal = ToStruct(binormalNext);
				vs[3].Tangent = ToStruct(tangentNext);
				vs[3].Binormal = ToStruct(binormalNext);

				vs[4].Tangent = ToStruct(tangentCurrent);
				vs[4].Binormal = ToStruct(binormalCurrent);
				vs[5].Tangent = ToStruct(tangentCurrent);
				vs[5].Binormal = ToStruct(binormalCurrent);
				vs[6].Tangent = ToStruct(tangentNext);
				vs[6].Binormal = ToStruct(binormalNext);
				vs[7].Tangent = ToStruct(tangentNext);
				vs[7].Binormal = ToStruct(binormalNext);
			}
			else if (vertexType == VertexType::Dynamic || vertexType == VertexType::Lighting)
			{
				StrideView<VERTEX> vs(&verteies[i], stride_, 8);

				// return back
				float t_b;
				t_b = old_c * (stepC)-old_s * (-stepS);
				auto s_b = old_s * (stepC) + old_c * (-stepS);
				auto c_b = t_b;

				::Effekseer::Vec3f outerBefore{
					c_b * outerRadius,
					s_b * outerRadius,
					outerHeight};

				// next
				auto t_n = cos_ * stepC - sin_ * stepS;
				auto s_n = sin_ * stepC + cos_ * stepS;
				auto c_n = t_n;

				::Effekseer::Vec3f outerNN;
				outerNN.SetX(c_n * outerRadius);
				outerNN.SetY(s_n * outerRadius);
				outerNN.SetZ(outerHeight);

				::Effekseer::Vec3f tangent0 = (outerCurrent - outerBefore).Normalize();
				::Effekseer::Vec3f tangent1 = (outerNext - outerCurrent).Normalize();
				::Effekseer::Vec3f tangent2 = (outerNN - outerNext).Normalize();

				auto tangentCurrent = (tangent0 + tangent1) / 2.0f;
				auto tangentNext = (tangent1 + tangent2) / 2.0f;

				auto binormalCurrent = v[5].Pos - v[0].Pos;
				auto binormalNext = v[7].Pos - v[2].Pos;

				::Effekseer::Vec3f normalCurrent;
				::Effekseer::Vec3f normalNext;

				normalCurrent = ::Effekseer::Vec3f::Cross(tangentCurrent, binormalCurrent);
				normalNext = ::Effekseer::Vec3f::Cross(tangentNext, binormalNext);

				if (!parameter.IsRightHand)
				{
					normalCurrent = -normalCurrent;
					normalNext = -normalNext;
				}

				// rotate directions
				::Effekseer::Mat43f matRot = mat43;
				matRot.SetTranslation({0.0f, 0.0f, 0.0f});

				normalCurrent = ::Effekseer::Vec3f::Transform(normalCurrent, matRot);
				normalNext = ::Effekseer::Vec3f::Transform(normalNext, matRot);
				tangentCurrent = ::Effekseer::Vec3f::Transform(tangentCurrent, matRot);
				tangentNext = ::Effekseer::Vec3f::Transform(tangentNext, matRot);

				normalCurrent = normalCurrent.Normalize();
				normalNext = normalNext.Normalize();
				tangentCurrent = tangentCurrent.Normalize();
				tangentNext = tangentNext.Normalize();

				const auto packedNormalCurrent = PackVector3DF(normalCurrent);
				const auto packedNormalNext = PackVector3DF(normalNext);
				const auto packedTangentCurrent = PackVector3DF(tangentCurrent);
				const auto packedTangentNext = PackVector3DF(tangentNext);

				vs[0].SetPackedNormal(packedNormalCurrent);
				vs[1].SetPackedNormal(packedNormalCurrent);
				vs[2].SetPackedNormal(packedNormalNext);
				vs[3].SetPackedNormal(packedNormalNext);

				vs[4].SetPackedNormal(packedNormalCurrent);
				vs[5].SetPackedNormal(packedNormalCurrent);
				vs[6].SetPackedNormal(packedNormalNext);
				vs[7].SetPackedNormal(packedNormalNext);

				vs[0].SetPackedTangent(packedTangentCurrent);
				vs[1].SetPackedTangent(packedTangentCurrent);
				vs[2].SetPackedTangent(packedTangentNext);
				vs[3].SetPackedTangent(packedTangentNext);

				vs[4].SetPackedTangent(packedTangentCurrent);
				vs[5].SetPackedTangent(packedTangentCurrent);
				vs[6].SetPackedTangent(packedTangentNext);
				vs[7].SetPackedTangent(packedTangentNext);

				// uv1
				uv1texNext = uv1Current + uv1Step;

				vs[0].SetUV2(uv1Current, uv1v1);
				vs[1].SetUV2(uv1Current, uv1v2);
				vs[2].SetUV2(uv1texNext, uv1v1);
				vs[3].SetUV2(uv1texNext, uv1v2);
				 
				vs[4].SetUV2(uv1Current, uv1v2);
				 
				vs[5].SetUV2(uv1Current, uv1v3);
				 
				vs[6].SetUV2(uv1texNext, uv1v2);
				
				vs[7].SetUV2(uv1texNext, uv1v3);
			}

			outerCurrent = outerNext;
			innerCurrent = innerNext;
			centerCurrent = centerNext;
			uv0Current = uv0texNext;
			uv1Current = uv1texNext;
#ifdef __EFFEKSEER_BUILD_VERSION16__
			for (int32_t uvi = 0; uvi < advancedUVNum; uvi++)
			{
				advancedUVCurrent[uvi] = advancedUVtexNext[uvi];
			}
#endif
			outerColor = outerColorNext;
			innerColor = innerColorNext;
			centerColor = centerColorNext;
		}

		if (CanSingleRendering())
		{
			m_singleRenderingMatrix = mat43;
		}
		else
		{
			TransformVertexes(verteies, singleVertexCount, mat43);
		}

		// custom parameter
		if (customData1Count_ > 0)
		{
			StrideView<float> custom(m_ringBufferData + sizeof(DynamicVertex), stride_, singleVertexCount);
			for (int i = 0; i < singleVertexCount; i++)
			{
				auto c = (float*)(&custom[i]);
				memcpy(c, instanceParameter.CustomData1.data(), sizeof(float) * customData1Count_);
			}
		}

		if (customData2Count_ > 0)
		{
			StrideView<float> custom(
				m_ringBufferData + sizeof(DynamicVertex) + sizeof(float) * customData1Count_, stride_, singleVertexCount);
			for (int i = 0; i < singleVertexCount; i++)
			{
				auto c = (float*)(&custom[i]);
				memcpy(c, instanceParameter.CustomData2.data(), sizeof(float) * customData2Count_);
			}
		}

		m_spriteCount += 2 * parameter.VertexCount;
		m_ringBufferData += stride_ * singleVertexCount;

	}

	void EndRendering_(RENDERER* renderer, const efkRingNodeParam& param, void* userData, const ::Effekseer::Mat44f& camera)
	{
		if (CanSingleRendering())
		{
			::Effekseer::Mat44f mat = m_singleRenderingMatrix * renderer->GetCameraMatrix();

			renderer->GetStandardRenderer()->Rendering(mat, renderer->GetProjectionMatrix());
		}

		if (param.DepthParameterPtr->ZSort != Effekseer::ZSortType::None && !CanSingleRendering())
		{
			for (auto& kv : instances_)
			{
				efkVector3D t = kv.Value.SRTMatrix43.GetTranslation();

				Effekseer::Vec3f frontDirection = m_renderer->GetCameraFrontDirection();
				if (!param.IsRightHand)
				{
					frontDirection.SetZ(-frontDirection.GetZ());
				}

				kv.Key = Effekseer::Vec3f::Dot(t, frontDirection);
			}

			if (param.DepthParameterPtr->ZSort == Effekseer::ZSortType::NormalOrder)
			{
				std::sort(instances_.begin(), instances_.end(), [](const KeyValue& a, const KeyValue& b) -> bool { return a.Key < b.Key; });
			}
			else
			{
				std::sort(instances_.begin(), instances_.end(), [](const KeyValue& a, const KeyValue& b) -> bool { return a.Key > b.Key; });
			}


			const auto& state = m_renderer->GetStandardRenderer()->GetState();

			for (auto& kv : instances_)
			{
				RenderingInstance(kv.Value, param, state, camera);
			}
			
		}
	}

public:
	void BeginRendering(const efkRingNodeParam& parameter, int32_t count, void* userData)
	{
		BeginRendering_(m_renderer, count, parameter);
	}

	void Rendering(const efkRingNodeParam& parameter, const efkRingInstanceParam& instanceParameter, void* userData)
	{
		if (m_spriteCount == m_renderer->GetSquareMaxCount()) return;
		Rendering_(parameter, instanceParameter, userData, m_renderer->GetCameraMatrix());
	}

	void EndRendering(const efkRingNodeParam& parameter, void* userData)
	{
		if (m_ringBufferData == NULL) return;

		if (m_spriteCount == 0 && parameter.DepthParameterPtr->ZSort == Effekseer::ZSortType::None)
			return;

		EndRendering_(m_renderer, parameter, userData, m_renderer->GetCameraMatrix());
	}
};
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
}
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
#endif	// __EFFEKSEERRENDERER_RING_RENDERER_H__