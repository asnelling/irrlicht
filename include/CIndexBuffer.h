// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_INDEX_BUFFER_H_INCLUDED__
#define __C_INDEX_BUFFER_H_INCLUDED__

#include "IIndexBuffer.h"

namespace irr
{
namespace scene
{
	class CIndexBuffer : public IIndexBuffer
	{
	public:
		CIndexBuffer(video::E_INDEX_TYPE pType = video::EIT_16BIT) : Indices(0), Type(pType), HardwareMappingHint(EHM_NEVER), ChangedID(1)
		{
			if(Type == video::EIT_32BIT)
				Indices = new CIndexList<u32>();
			else // EIT_16BIT
				Indices = new CIndexList<u16>();
		}

		CIndexBuffer(const CIndexBuffer &pIndexBuffer) : Indices(0), ChangedID(1)
		{
			Type = pIndexBuffer.getType();

			HardwareMappingHint = pIndexBuffer.getHardwareMappingHint();

			if(Type == video::EIT_32BIT)
				Indices = new CIndexList<u32>();
			else // EIT_16BIT
				Indices = new CIndexList<u16>();

			Indices->reallocate(pIndexBuffer.getIndexCount());

			for(u32 i = 0; i < pIndexBuffer.getIndexCount(); ++i)
				addIndex(pIndexBuffer.getIndex(i));
		}

		virtual ~CIndexBuffer()
		{
			delete Indices;
		}

		virtual void clear()
		{
			Indices->clear();
		}

		virtual u32 getLast()
		{
			return (u32)Indices->getLast();
		}

		virtual void set_used(u32 pUsed)
		{
			Indices->set_used(pUsed);
		}

		virtual void reallocate(u32 pSize)
		{
			Indices->reallocate(pSize);
		}

		virtual u32 allocated_size() const
		{
			return Indices->allocated_size();
		}

		virtual s32 linear_reverse_search(const u32& pElement) const
		{
			return Indices->linear_reverse_search(pElement);
		}

		virtual video::E_INDEX_TYPE getType() const
		{
			return Type;
		}

		virtual void setType(video::E_INDEX_TYPE pType)
		{
			if(Type == pType)
				return;

			Type = pType;

			IIndexList* Indices = 0;

			switch(Type)
			{
				case video::EIT_16BIT:
				{
					Indices = new CIndexList<u16>();
					break;
				}
				case video::EIT_32BIT:
				{
					Indices = new CIndexList<u32>();
					break;
				}
			}

			if(Indices)
			{
				Indices->reallocate(Indices->size());

				for(u32 i = 0; i < Indices->size(); ++i)
					Indices->addIndex(Indices->getIndex(i));

				delete Indices;
			}

			Indices = Indices;
		}

		virtual E_HARDWARE_MAPPING getHardwareMappingHint() const
		{
			return HardwareMappingHint;
		}

		virtual void setHardwareMappingHint(E_HARDWARE_MAPPING pHardwareMappingHint)
		{
			if(HardwareMappingHint != pHardwareMappingHint)
				setDirty();

			HardwareMappingHint = pHardwareMappingHint;
		}

		virtual void addIndex(const u32& pIndex)
		{
			Indices->addIndex(pIndex);
		}

		virtual u32 getIndex(u32 pID) const
		{
			return Indices->getIndex(pID);
		}

		virtual void* getIndices()
		{
			return Indices->pointer();
		}

		virtual u32 getIndexCount() const
		{
			return Indices->size();
		}

		virtual u32 getIndexSize() const
		{
			if(Type == video::EIT_32BIT)
				return sizeof(u32);

			return sizeof(u16);
		}

		virtual void setIndex(u32 pID, u32 pValue)
		{
			Indices->setIndex(pID, pValue);
		}		

		virtual void setDirty()
		{
			++ChangedID;
		}

		virtual u32 getChangedID() const
		{
			return ChangedID;
		}

	protected:
		class IIndexList
		{
		public:
			virtual void clear() = 0;
			virtual void* pointer() = 0;
			virtual u32 size() const = 0;
			virtual u32 getLast() = 0;
			virtual void set_used(u32 pUsed) = 0;
			virtual void reallocate(u32 pSize) = 0;
			virtual u32 allocated_size() const = 0;
			virtual s32 linear_reverse_search(const u32& pElement) const = 0;
			virtual void addIndex(const u32& pIndex) = 0;
			virtual u32 getIndex(u32 pID) const = 0;
			virtual void setIndex(u32 pID, u32 pValue) = 0;
		};

		template <class T>
		class CIndexList : public IIndexList
		{
		public:
			CIndexList() : Data(0)
			{
			}

			CIndexList(const CIndexList &pIndexList) : Data(0)
			{
				Data.reallocate(pIndexList.Data.size());

				for(u32 i = 0; i < pIndexList.Data.size(); ++i)
					Data.push_back(pIndexList.Data[i]);
			}

			~CIndexList()
			{
				Data.clear();
			}

			virtual void clear()
			{
				Data.clear();
			}

			virtual void* pointer()
			{
				return Data.pointer();
			}

			virtual u32 size() const
			{
				return Data.size();
			}

			virtual u32 getLast()
			{
				return (u32)Data.getLast();
			}

			virtual void set_used(u32 pUsed)
			{
				Data.set_used(pUsed);
			}

			virtual void reallocate(u32 pSize)
			{
				Data.reallocate(pSize);
			}

			virtual u32 allocated_size() const
			{
				return Data.allocated_size();
			}

			virtual s32 linear_reverse_search(const u32& pElement) const
			{
				return Data.linear_reverse_search(pElement);
			}

			virtual void addIndex(const u32& pIndex)
			{
				Data.push_back(pIndex);
			}

			virtual u32 getIndex(u32 pID) const
			{
				if(pID < Data.size())
					return Data[pID];

				return 0;
			}

			virtual void setIndex(u32 pID, u32 pValue)
			{
				if(pID < Data.size())
					Data[pID] = (T)pValue;
			}	

		protected:
			core::array<T> Data;
		};

		IIndexList* Indices;

		video::E_INDEX_TYPE Type;

		E_HARDWARE_MAPPING HardwareMappingHint;

		u32 ChangedID;
	};
}
}

#endif
