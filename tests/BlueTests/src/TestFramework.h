#pragma once

#include <Blue/System/Assert.h>

#include <stdio.h>

#define BLUE_TEST( name ) static bool name( )
#define BLUE_EXPECT_TRUE( expression )                                                                                 \
  do                                                                                                                   \
  {                                                                                                                    \
    if ( !( expression ) )                                                                                             \
    {                                                                                                                  \
      printf( "[  ASSERT  ] %s\n", #expression );                                                                      \
      printf( "[ LOCATION ] %s:%d\n", __FILE__, __LINE__ );                                                            \
      fflush( stdout );                                                                                                \
      return false;                                                                                                    \
    }                                                                                                                  \
  }                                                                                                                    \
  while ( false )

#define BLUE_EXPECT_EQ( left, right ) BLUE_EXPECT_TRUE( ( left ) == ( right ) )
