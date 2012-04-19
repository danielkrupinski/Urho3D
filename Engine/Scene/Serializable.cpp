//
// Urho3D Engine
// Copyright (c) 2008-2012 Lasse ��rni
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"
#include "Context.h"
#include "Deserializer.h"
#include "Log.h"
#include "ReplicationState.h"
#include "Serializable.h"
#include "Serializer.h"
#include "XMLElement.h"

#include "DebugNew.h"

OBJECTTYPESTATIC(Serializable);

Serializable::Serializable(Context* context) :
    Object(context),
    loading_(false)
{
}

Serializable::~Serializable()
{
}

void Serializable::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    // Check for accessor function mode
    if (attr.accessor_)
    {
        attr.accessor_->Set(this, src);
        return;
    }
    
    // Calculate the destination address
    void* dest = reinterpret_cast<unsigned char*>(this) + attr.offset_;
    
    switch (attr.type_)
    {
    case VAR_INT:
        // If enum type, use the low 8 bits only (assume full value to be initialized)
        if (attr.enumNames_)
            *(reinterpret_cast<unsigned char*>(dest)) = src.GetInt();
        else
            *(reinterpret_cast<int*>(dest)) = src.GetInt();
        break;
        
    case VAR_BOOL:
        *(reinterpret_cast<bool*>(dest)) = src.GetBool();
        break;
        
    case VAR_FLOAT:
        *(reinterpret_cast<float*>(dest)) = src.GetFloat();
        break;
        
    case VAR_VECTOR2:
        *(reinterpret_cast<Vector2*>(dest)) = src.GetVector2();
        break;
        
    case VAR_VECTOR3:
        *(reinterpret_cast<Vector3*>(dest)) = src.GetVector3();
        break;
        
    case VAR_VECTOR4:
        *(reinterpret_cast<Vector4*>(dest)) = src.GetVector4();
        break;
        
    case VAR_QUATERNION:
        *(reinterpret_cast<Quaternion*>(dest)) = src.GetQuaternion();
        break;
        
    case VAR_COLOR:
        *(reinterpret_cast<Color*>(dest)) = src.GetColor();
        break;
        
    case VAR_STRING:
        *(reinterpret_cast<String*>(dest)) = src.GetString();
        break;
        
    case VAR_BUFFER:
        *(reinterpret_cast<PODVector<unsigned char>*>(dest)) = src.GetBuffer();
        break;
        
    case VAR_RESOURCEREF:
        *(reinterpret_cast<ResourceRef*>(dest)) = src.GetResourceRef();
        break;
        
    case VAR_RESOURCEREFLIST:
        *(reinterpret_cast<ResourceRefList*>(dest)) = src.GetResourceRefList();
        break;
        
    case VAR_VARIANTVECTOR:
        *(reinterpret_cast<VariantVector*>(dest)) = src.GetVariantVector();
        break;
        
    case VAR_VARIANTMAP:
        *(reinterpret_cast<VariantMap*>(dest)) = src.GetVariantMap();
        break;
    }
}

void Serializable::OnGetAttribute(const AttributeInfo& attr, Variant& dest)
{
    // Check for accessor function mode
    if (attr.accessor_)
    {
        attr.accessor_->Get(this, dest);
        return;
    }
    
    // Calculate the source address
    void* src = reinterpret_cast<unsigned char*>(this) + attr.offset_;
    
    switch (attr.type_)
    {
    case VAR_INT:
        // If enum type, use the low 8 bits only
        if (attr.enumNames_)
            dest = *(reinterpret_cast<const unsigned char*>(src));
        else
            dest = *(reinterpret_cast<const int*>(src));
        break;
        
    case VAR_BOOL:
        dest = *(reinterpret_cast<const bool*>(src));
        break;
        
    case VAR_FLOAT:
        dest = *(reinterpret_cast<const float*>(src));
        break;
        
    case VAR_VECTOR2:
        dest = *(reinterpret_cast<const Vector2*>(src));
        break;
        
    case VAR_VECTOR3:
        dest = *(reinterpret_cast<const Vector3*>(src));
        break;
        
    case VAR_VECTOR4:
        dest = *(reinterpret_cast<const Vector4*>(src));
        break;
        
    case VAR_QUATERNION:
        dest = *(reinterpret_cast<const Quaternion*>(src));
        break;
        
    case VAR_COLOR:
        dest = *(reinterpret_cast<const Color*>(src));
        break;
        
    case VAR_STRING:
        dest = *(reinterpret_cast<const String*>(src));
        break;
        
    case VAR_BUFFER:
        dest = *(reinterpret_cast<const PODVector<unsigned char>*>(src));
        break;
        
    case VAR_RESOURCEREF:
        dest = *(reinterpret_cast<const ResourceRef*>(src));
        break;
        
    case VAR_RESOURCEREFLIST:
        dest = *(reinterpret_cast<const ResourceRefList*>(src));
        break;
        
    case VAR_VARIANTVECTOR:
        dest = *(reinterpret_cast<const VariantVector*>(src));
        break;
        
    case VAR_VARIANTMAP:
        dest = *(reinterpret_cast<const VariantMap*>(src));
        break;
    }
}

bool Serializable::Load(Deserializer& source)
{
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes)
        return true;
    
    loading_ = true;
    
    for (unsigned i = 0; i < attributes->Size(); ++i)
    {
        const AttributeInfo& attr = attributes->At(i);
        if (!(attr.mode_ & AM_FILE))
            continue;
        
        if (source.IsEof())
        {
            LOGERROR("Could not load " + GetTypeName() + ", stream not open or at end");
            loading_  = false;
            return false;
        }
        
        OnSetAttribute(attr, source.ReadVariant(attr.type_));
    }
    
    loading_ = false;
    return true;
}

bool Serializable::Save(Serializer& dest)
{
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes)
        return true;
    
    Variant value;
    
    for (unsigned i = 0; i < attributes->Size(); ++i)
    {
        const AttributeInfo& attr = attributes->At(i);
        if (!(attr.mode_ & AM_FILE))
            continue;
        
        OnGetAttribute(attr, value);
        if (!dest.WriteVariantData(value))
        {
            LOGERROR("Could not save " + GetTypeName() + ", writing to stream failed");
            return false;
        }
    }
    
    return true;
}

bool Serializable::LoadXML(const XMLElement& source)
{
    if (source.IsNull())
    {
        LOGERROR("Could not load " + GetTypeName() + ", null source element");
        return false;
    }
    
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes)
        return true;
    
    loading_ = true;
    
    XMLElement attrElem = source.GetChild("attribute");
    unsigned startIndex = 0;
    
    while (attrElem)
    {
        const char* name = attrElem.GetAttribute("name");
        unsigned i = startIndex;
        unsigned attempts = attributes->Size();
        
        while (attempts)
        {
            const AttributeInfo& attr = attributes->At(i);
            if ((attr.mode_ & AM_FILE) && attr.name_ == name)
            {
                // If enums specified, do enum lookup and int assignment. Otherwise assign the variant directly
                if (attr.enumNames_)
                {
                    const char* value = attrElem.GetAttribute("value");
                    const String* enumPtr = attr.enumNames_;
                    int enumValue = 0;
                    bool enumFound = false;
                    while (enumPtr->Length())
                    {
                        if (*enumPtr == value)
                        {
                            enumFound = true;
                            break;
                        }
                        ++enumPtr;
                        ++enumValue;
                    }
                    if (enumFound)
                        OnSetAttribute(attr, Variant(enumValue));
                    else
                        LOGWARNING("Unknown enum value " + String(value) + " in attribute " + String(attr.name_));
                }
                else
                    OnSetAttribute(attr, attrElem.GetVariantValue(attr.type_));
                
                startIndex = (i + 1) % attributes->Size();
                break;
            }
            else
            {
                i = (i + 1) % attributes->Size();
                --attempts;
            }
        }
        
        if (!attempts)
            LOGWARNING("Unknown attribute " + String(name) + " in XML data");
        
        attrElem = attrElem.GetNext("attribute");
    }
    
    loading_ = false;
    return true;
}

bool Serializable::SaveXML(XMLElement& dest)
{
    if (dest.IsNull())
    {
        LOGERROR("Could not save " + GetTypeName() + ", null destination element");
        return false;
    }
    
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes)
        return true;
    
    Variant value;
    
    for (unsigned i = 0; i < attributes->Size(); ++i)
    {
        const AttributeInfo& attr = attributes->At(i);
        if (!(attr.mode_ & AM_FILE))
            continue;
        
        XMLElement attrElem = dest.CreateChild("attribute");
        attrElem.SetAttribute("name", attr.name_.CString());
        OnGetAttribute(attr, value);
        // If enums specified, set as an enum string. Otherwise set directly as a Variant
        if (attr.enumNames_)
        {
            int enumValue = value.GetInt();
            attrElem.SetAttribute("value", attr.enumNames_[enumValue].CString());
        }
        else
            attrElem.SetVariantValue(value);
    }
    
    return true;
}

bool Serializable::SetAttribute(unsigned index, const Variant& value)
{
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes)
    {
        LOGERROR(GetTypeName() + " has no attributes");
        return false;
    }
    if (index >= attributes->Size())
    {
        LOGERROR("Attribute index out of bounds");
        return false;
    }
    
    const AttributeInfo& attr = attributes->At(index);
    
    // Check that the new value's type matches the attribute type
    if (value.GetType() == attr.type_)
    {
        OnSetAttribute(attr, value);
        return true;
    }
    else
    {
        LOGERROR("Could not set attribute " + String(attr.name_) + ": expected type " + Variant::GetTypeName(attr.type_) +
            " but got " + value.GetTypeName());
        return false;
    }
}

bool Serializable::SetAttribute(const String& name, const Variant& value)
{
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes)
    {
        LOGERROR(GetTypeName() + " has no attributes");
        return false;
    }
    
    for (Vector<AttributeInfo>::ConstIterator i = attributes->Begin(); i != attributes->End(); ++i)
    {
        if (i->name_ == name)
        {
            // Check that the new value's type matches the attribute type
            if (value.GetType() == i->type_)
            {
                OnSetAttribute(*i, value);
                return true;
            }
            else
            {
                LOGERROR("Could not set attribute " + String(i->name_) + ": expected type " + Variant::GetTypeName(i->type_)
                    + " but got " + value.GetTypeName());
                return false;
            }
        }
    }
    
    LOGERROR("Could not find attribute " + String(name) + " in " + GetTypeName());
    return false;
}

void Serializable::WriteInitialDeltaUpdate(Serializer& dest)
{
    const Vector<AttributeInfo>* attributes = GetNetworkAttributes();
    if (!attributes)
        return;
    
    unsigned numAttributes = attributes->Size();
    DirtyBits attributeBits;
    
    // Compare against defaults
    for (unsigned i = 0; i < numAttributes; ++i)
    {
        const AttributeInfo& attr = attributes->At(i);
        if (currentState_[i] != attr.defaultValue_)
            attributeBits.Set(i);
    }
    
    // First write the change bitfield, then attribute data for non-default attributes
    dest.Write(attributeBits.data_, (numAttributes + 7) >> 3);
    
    for (unsigned i = 0; i < numAttributes; ++i)
    {
        if (attributeBits.IsSet(i))
            dest.WriteVariantData(currentState_[i]);
    }
}

void Serializable::WriteDeltaUpdate(Serializer& dest, const DirtyBits& attributeBits)
{
    const Vector<AttributeInfo>* attributes = GetNetworkAttributes();
    if (!attributes)
        return;
    
    unsigned numAttributes = attributes->Size();
    
    // First write the change bitfield, then attribute data for changed attributes
    // Note: the attribute bits should not contain LATESTDATA attributes
    dest.Write(attributeBits.data_, (numAttributes + 7) >> 3);
    
    for (unsigned i = 0; i < numAttributes; ++i)
    {
        if (attributeBits.IsSet(i))
            dest.WriteVariantData(currentState_[i]);
    }
}

void Serializable::WriteLatestDataUpdate(Serializer& dest)
{
    const Vector<AttributeInfo>* attributes = GetNetworkAttributes();
    if (!attributes)
        return;
    
    unsigned numAttributes = attributes->Size();
    
    for (unsigned i = 0; i < numAttributes; ++i)
    {
        if (attributes->At(i).mode_ & AM_LATESTDATA)
            dest.WriteVariantData(currentState_[i]);
    }
}

void Serializable::ReadDeltaUpdate(Deserializer& source)
{
    const Vector<AttributeInfo>* attributes = GetNetworkAttributes();
    if (!attributes)
        return;
    
    unsigned numAttributes = attributes->Size();
    DirtyBits attributeBits;
    
    source.Read(attributeBits.data_, (numAttributes + 7) >> 3);
    
    for (unsigned i = 0; i < numAttributes && !source.IsEof(); ++i)
    {
        if (attributeBits.IsSet(i))
        {
            const AttributeInfo& attr = attributes->At(i);
            OnSetAttribute(attr, source.ReadVariant(attr.type_));
        }
    }
}

void Serializable::ReadLatestDataUpdate(Deserializer& source)
{
    const Vector<AttributeInfo>* attributes = GetNetworkAttributes();
    if (!attributes)
        return;
    
    unsigned numAttributes = attributes->Size();
    
    for (unsigned i = 0; i < numAttributes && !source.IsEof(); ++i)
    {
        const AttributeInfo& attr = attributes->At(i);
        if (attr.mode_ & AM_LATESTDATA)
            OnSetAttribute(attr, source.ReadVariant(attr.type_));
    }
}

Variant Serializable::GetAttribute(unsigned index)
{
    Variant ret;
    
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes || index >= attributes->Size())
        return ret;
    
    OnGetAttribute(attributes->At(index), ret);
    return ret;
}

Variant Serializable::GetAttribute(const String& name)
{
    Variant ret;
    
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    if (!attributes)
    {
        LOGERROR(GetTypeName() + " has no attributes");
        return ret;
    }
    
    for (Vector<AttributeInfo>::ConstIterator i = attributes->Begin(); i != attributes->End(); ++i)
    {
        if (i->name_ == name)
        {
            OnGetAttribute(*i, ret);
            return ret;
        }
    }
    
    LOGERROR("Could not find attribute " + String(name) + " in " + GetTypeName());
    return ret;
}

unsigned Serializable::GetNumAttributes() const
{
    const Vector<AttributeInfo>* attributes = context_->GetAttributes(GetType());
    return attributes ? attributes->Size() : 0;
}

unsigned Serializable::GetNumNetworkAttributes() const
{
    const Vector<AttributeInfo>* attributes = context_->GetNetworkAttributes(GetType());
    return attributes ? attributes->Size() : 0;
}

const Vector<AttributeInfo>* Serializable::GetAttributes() const
{
    return context_->GetAttributes(GetType());
}

const Vector<AttributeInfo>* Serializable::GetNetworkAttributes() const
{
    return context_->GetNetworkAttributes(GetType());
}
