#pragma once

#include <vector>
#include "tbb/tbb.h"

namespace IECore
{
namespace Private
{
//! counts aren't very helpful for parallel processing of data
//! create an offset index into the vertex array by creating a prefix sum of the vertex count per face
template<typename T>
struct CalculateOffsetsFromCounts
{
	CalculateOffsetsFromCounts(
		const std::vector<T> &counts,
		std::vector<int> &offsets
	) : m_counts( counts ), m_offsets( offsets ), m_acc( 0 )
	{
	}

	CalculateOffsetsFromCounts( CalculateOffsetsFromCounts &other, tbb::split ) : m_counts( other.m_counts ), m_offsets( other.m_offsets ), m_acc( 0 )
	{
	}

	void reverse_join( CalculateOffsetsFromCounts &other )
	{
		m_acc = other.m_acc + m_acc;
	}

	void assign( CalculateOffsetsFromCounts &other )
	{
		m_acc = other.m_acc;
	}

	template<typename Tag>
	void operator()( const tbb::blocked_range <size_t> &r, Tag )
	{
		int acc = m_acc;
		for( size_t i = r.begin(); i != r.end(); ++i )
		{
			if( Tag::is_final_scan() )
				m_offsets[i] = acc;

			acc += m_counts[i];
		}

		m_acc = acc;
	}

	const std::vector <T> &m_counts;
	std::vector<int> &m_offsets;
	int m_acc;
};

}
}