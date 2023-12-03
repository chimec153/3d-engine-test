#include "Material.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"

namespace Engine
{
	Material::Material() :
		Bindable()
		, m_tMaterial()
		, m_pConstantBuffer(StaticFindBindable<ConstantBuffer<MATERIAL>>("Material"))
	{
		SetBindableType(BINDABLE_TYPE::MATERIAL);

		m_tMaterial.diffuseColor.x = 1.f;
		m_tMaterial.diffuseColor.y = 1.f;
		m_tMaterial.diffuseColor.z = 1.f;
		m_tMaterial.diffuseColor.w = 1.f;

		m_tMaterial.ambientColor.x = 1.f;
		m_tMaterial.ambientColor.y = 1.f;
		m_tMaterial.ambientColor.z = 1.f;
		m_tMaterial.ambientColor.w = 1.f;

		m_tMaterial.specularColor.x = 0.8f;
		m_tMaterial.specularColor.y = 0.2f;
		m_tMaterial.specularColor.z = 0.1f;
		m_tMaterial.specularColor.w = 1.f;

		m_tMaterial.emissiveColor.x = 1.f;
		m_tMaterial.emissiveColor.y = 1.f;
		m_tMaterial.emissiveColor.z = 1.f;
		m_tMaterial.emissiveColor.w = 1.f;

		m_tMaterial.fSpecPower = 100;
		m_tMaterial.fFraction = 0.5f;
		m_tMaterial.vRoughness.x = 0.5f;
		m_tMaterial.vRoughness.y = 0.1f;
	}

	Material::Material(const std::shared_ptr<ConstantBuffer<MATERIAL>>& pBuffer) :
		Bindable()
		, m_tMaterial()
		, m_pConstantBuffer(pBuffer)
	{
		SetBindableType(BINDABLE_TYPE::MATERIAL);

		m_tMaterial.diffuseColor.w = 1.f;
		m_tMaterial.specularColor.w = 1.f;
	}

	Material::Material(const Material& material) :
		Bindable(material)
		, m_tMaterial(material.m_tMaterial)
		, m_pConstantBuffer(material.m_pConstantBuffer)
	{
	}

	void Material::SetDiffuseColor(float r, float g, float b, float w)
	{
		m_tMaterial.diffuseColor.x = r;
		m_tMaterial.diffuseColor.y = g;
		m_tMaterial.diffuseColor.z = b;
		m_tMaterial.diffuseColor.w = w;
	}

	void Material::SetAmbientColor(float r, float g, float b, float w)
	{
		m_tMaterial.ambientColor.x = r;
		m_tMaterial.ambientColor.y = g;
		m_tMaterial.ambientColor.z = b;
		m_tMaterial.ambientColor.w = w;
	}

	void Material::SetSpecularColor(float r, float g, float b, float w)
	{
		m_tMaterial.specularColor.x = r;
		m_tMaterial.specularColor.y = g;
		m_tMaterial.specularColor.z = b;
		m_tMaterial.specularColor.w = w;
	}

	void Material::SetDiffuseColor(const Vector4& color)
	{
		m_tMaterial.diffuseColor = color;
	}

	void Material::SetAmbientColor(const Vector4& color)
	{
		m_tMaterial.ambientColor = color;
	}

	void Material::SetSpecularColor(const Vector4& color)
	{
		m_tMaterial.specularColor = color;
	}

	void Material::SetEmissiveColor(const Vector4& color)
	{
		m_tMaterial.emissiveColor = color;
	}

	void Material::SetShininess(float fShininess)
	{
		m_tMaterial.fSpecPower = fShininess;
	}

	void Material::SetReflectivity(float fReflectivity)
	{
		m_tMaterial.fFraction = fReflectivity;
	}

	void Material::SetRandomColor()
	{
		m_tMaterial.diffuseColor.x = rand() % 256 / 255.f;
		m_tMaterial.diffuseColor.y = rand() % 256 / 255.f;
		m_tMaterial.diffuseColor.z = rand() % 256 / 255.f;
		m_tMaterial.diffuseColor.w = 1.f;

		m_tMaterial.ambientColor.x = rand() % 256 / 255.f;
		m_tMaterial.ambientColor.y = rand() % 256 / 255.f;
		m_tMaterial.ambientColor.z = rand() % 256 / 255.f;
		m_tMaterial.ambientColor.w = 1.f;

		m_tMaterial.specularColor.x = rand() % 256 / 255.f;
		m_tMaterial.specularColor.y = rand() % 256 / 255.f;
		m_tMaterial.specularColor.z = rand() % 256 / 255.f;
		m_tMaterial.specularColor.w = 1.f;

		m_tMaterial.fSpecPower = rand() % 200 + 1.f;
		m_tMaterial.fFraction = rand() % 201 / 200.f;
		m_tMaterial.vRoughness.x = rand() % 201 / 200.f;
		m_tMaterial.vRoughness.y = rand() % 201 / 200.f;
	}

	const MATERIAL& Material::GetMaterial() const
	{
		return m_tMaterial;
	}

	void Material::SetMaterial(const MATERIAL& mtrl)
	{
		m_tMaterial = mtrl;
	}

	void Material::SetRoughnessX(float x)
	{
		m_tMaterial.vRoughness.x = x;
	}

	void Material::SetRoughnessY(float y)
	{
		m_tMaterial.vRoughness.y = y;
	}

	void Material::Update(float fDeltaTime)
	{
	}

	void Material::Bind()
	{
		m_pConstantBuffer->UpdateBuffer(m_tMaterial);

		m_pConstantBuffer->Bind();
	}

	std::shared_ptr<Bindable> Material::Clone()
	{
		return std::make_shared<Material>(*this);
	}

	void Material::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_tMaterial, 80, 1, pFile);
	}

	void Material::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_tMaterial, 80, 1, pFile);
	}
}