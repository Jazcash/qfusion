#ifndef QFUSION_SND_PROPAGATION_H
#define QFUSION_SND_PROPAGATION_H

#include "snd_local.h"

class PropagationTable {
	friend class PropagationTableLoader;
	friend class PropagationTableBuilder;

	struct alignas( 1 )PropagationProps {
		int8_t dirX: 6;
		int8_t dirY: 6;
		int8_t dirZ: 6;
		uint8_t hasIndirectPath: 1;
		uint8_t hasDirectPath: 1;
		// 12 bits are left for the distance
		// These bits are split in 2 parts to fit uint8_t field type and thus do not enforce non-1 alignment.
		uint8_t distancePart1: 6;
		uint8_t distancePart2: 6;

		void SetDir( const vec3_t dir ) {
			// We have 5 bits for value and 6th for sign.
			// We do not want using DirToByte as it uses a sequential search.
			dirX = (int8_t)( dir[0] * 31.9f );
			dirY = (int8_t)( dir[1] * 31.9f );
			dirZ = (int8_t)( dir[2] * 31.9f );
		}

		void GetDir( vec3_t dir ) const {
			dir[0] = dirX / 32.0f;
			dir[1] = dirY / 32.0f;
			dir[2] = dirZ / 32.0f;
			VectorNormalize( dir );
		}

		void SetDistance( float distance ) {
			assert( distance >= 0 );
			// We have 12 bits for the distance.
			// Using rounding up to 16 units, we could store paths of length up to 2^16
			constexpr auto maxDistance = (float)( ( 1u << 16u ) - 1 );
			clamp_high( distance, maxDistance );
			const auto u = (unsigned)( distance / 16.0f );
			// Check whether the rounded distance really fits these 12 bits
			assert( u < ( 1u << 12u ) );
			// A mask for 6 bits
			constexpr unsigned mask = 077u;
			distancePart1 = (uint8_t)( ( u >> 6u ) & mask );
			distancePart2 = (uint8_t)( u & mask );
		}

		float GetDistance() const {
			return 16.0f * ( ( (uint32_t)distancePart1 << 6u ) | distancePart2 );
		}
	};

	static_assert( alignof( PropagationProps ) == 1, "" );

	const int numLeafs;
	PropagationProps *table { nullptr };

	const PropagationProps &GetProps( int fromLeafNum, int toLeafNum ) const {
		assert( table );
		assert( numLeafs );
		assert( fromLeafNum > 0 && fromLeafNum < numLeafs );
		assert( toLeafNum > 0 && toLeafNum < numLeafs );
		return table[numLeafs * fromLeafNum + toLeafNum];
	}
public:
	~PropagationTable() {
		if( table ) {
			S_Free( table );
		}
	}

	bool IsValid() const { return table != nullptr; }

	/**
	 * Returns true if a direct (ray-like) path between these leaves exists.
	 * @note true results of {@code HasDirectPath()} and {@code HasIndirectPath} are mutually exclusive.
	 */
	bool HasDirectPath( int fromLeafNum, int toLeafNum ) const {
		return fromLeafNum == toLeafNum || GetProps( fromLeafNum, toLeafNum ).hasDirectPath;
	}

	/**
	 * Returns true if an indirect (maze-like) path between these leaves exists.
	 * @note true results of {@code HasDirectPath()} and {@code HasIndirectPath} are mutually exclusive.
	 */
	bool HasIndirectPath( int fromLeafNum, int toLeafNum ) const {
		return fromLeafNum != toLeafNum && GetProps( fromLeafNum, toLeafNum ).hasIndirectPath;
	}

	/**
	 * Returns propagation properties of an indirect (maze) path between these leaves.
	 * @param fromLeafNum a number of leaf where a real sound emitter origin is assumed to be located.
	 * @param toLeafNum a number of leaf where a listener origin is assumed to be located.
	 * @param dir An average direction of sound waves emitted by the source and ingoing to the listener leaf.
	 * @param distance An estimation of distance that is covered by sound waves during propagation.
	 * @return true if an indirect path between given leaves exists (and there were propagation properties).
	 */
	bool GetIndirectPathProps( int fromLeafNum, int toLeafNum, vec3_t dir, float *distance ) const {
		if( fromLeafNum == toLeafNum ) {
			return false;
		}
		const auto &props = GetProps( fromLeafNum, toLeafNum );
		if( !props.hasIndirectPath ) {
			return false;
		}
		props.GetDir( dir );
		*distance = props.GetDistance();
		return true;
	}
};

#endif
