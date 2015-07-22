/* Copyright (c) 2011-2015, EPFL/Blue Brain Project
 *                     Ahmet Bilgili <ahmet.bilgili@epfl.ch>
 *                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *
 * This file is part of Livre <https://github.com/BlueBrain/Livre>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <livre/core/Cache/CacheStatistics.h>
#include <livre/core/Cache/CacheObject.h>
#include <livre/core/Util/ThreadClock.h>

namespace livre
{

struct CacheStatistics::LoadInfo
{
    enum Operation
    {
        OP_LOAD,
        OP_UNLOAD
    };

    LoadInfo( )
        : op( OP_LOAD ),
          cacheSize( 0 ),
          cumulativeCacheSize( 0 ),
          cumulativeNbNodes( 0 )
    {
        time = ThreadClock::getClock().getTimed();
    }

    LoadInfo(  const LoadInfo& previous,
               Operation operation,
               const uint32_t cSize,
               const float lTime = 0.0f )
        : op( operation ), cacheSize( cSize ), loadTime( lTime )
    {
        time = ThreadClock::getClock().getTimed();
        switch( op )
        {
        case OP_LOAD:
            cumulativeCacheSize = previous.cumulativeCacheSize + cacheSize;
            cumulativeNbNodes = previous.cumulativeNbNodes + 1;
            break;
        case OP_UNLOAD:
            cumulativeCacheSize = previous.cumulativeCacheSize - cacheSize;
            cumulativeNbNodes = previous.cumulativeNbNodes - 1;
            break;
        }
    }

    friend std::ostream& operator<<( std::ostream& stream, const LoadInfo& loadInfo )
    {
        stream << "Time: " << loadInfo.time
               << " Cumulative Nodes: " << loadInfo.cumulativeNbNodes
               << " Cumulative Size: " << loadInfo.cumulativeCacheSize;
        return stream;
    }

    Operation op;
    double time;
    uint32_t cacheSize;
    uint32_t cumulativeCacheSize;
    uint32_t cumulativeNbNodes;
    double loadTime;
};

CacheStatistics::CacheStatistics( const std::string& statisticsName,
                                  const uint32_t queueSize )
    : totalBlockCount_( 0 )
    , totalMemoryUsed_( 0 )
    , statisticsName_( statisticsName )
    , maxMemory_( 0 )
    , cacheHit_( 0 )
    , cacheMiss_( 0 )
    , queueSize_( queueSize )
{
}

void CacheStatistics::onLoaded_( const CacheObject& cacheObject )
{
   ++totalBlockCount_;
   totalMemoryUsed_ += cacheObject.getCacheSize() / LB_1MB;

   if( ioQueue_.empty() )
       ioQueue_.push( LoadInfoPtr( new LoadInfo()) );

   if( ioQueue_.getSize() == queueSize_ )
       ioQueue_.pop( );

   LoadInfoPtr previous;
   ioQueue_.getBack( previous );
   ioQueue_.push( LoadInfoPtr( new LoadInfo( *previous, LoadInfo::OP_LOAD,
                                             cacheObject.getCacheSize(),
                                             cacheObject.getLoadTime( ))));
}

void CacheStatistics::onPreUnload_( const CacheObject& cacheObject )
{
    --totalBlockCount_;
    totalMemoryUsed_ -= cacheObject.getCacheSize() / LB_1MB;

    if( ioQueue_.getSize() == queueSize_ )
        ioQueue_.pop( );

    LoadInfoPtr previous;
    ioQueue_.getBack( previous );
    ioQueue_.push( LoadInfoPtr( new LoadInfo( *previous, LoadInfo::OP_UNLOAD,
                                              cacheObject.getCacheSize( ))));
}

std::ostream& operator<<( std::ostream& stream, const CacheStatistics& cacheStatistics )
{
    stream << cacheStatistics.statisticsName_ << std::endl;
    stream << "  Total Used Memory: "
           << cacheStatistics.totalMemoryUsed_ << "/"
           << cacheStatistics.maxMemory_ << "MB" << std::endl;
    stream << "  Total Block Count: "
           << cacheStatistics.totalBlockCount_ << std::endl;
    stream << "  Cache hits: "
           << cacheStatistics.cacheHit_ << std::endl;
    stream << "  Cache misses: "
           << cacheStatistics.cacheMiss_ << std::endl;

    return stream;
}

CacheStatistics::~CacheStatistics()
{

}

}
