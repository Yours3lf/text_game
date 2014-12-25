#pragma once

#include <cmath>

//transition functions
//input:    x  [0...1]
//output: f(x) [0...1]
class transition
{
  public:
    typedef float (*func)(float);

    static float linear( float x )
    {
      return x;
    }

    static float quadratic_in( float x )
    {
      return x * x;
    }

    static float quadratic_out( float x )
    {
      return 1 - quadratic_in(1 - x);
    }

    static float quadratic_inout( float x )
    {
      if( x < 0.5 )
      {
        return quadratic_in( x * 2 ) * 0.5;
      }
      else
      {
        return quadratic_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float cubic_in( float x )
    {
      return x * x * x;
    }

    static float cubic_out( float x )
    {
      return 1 - cubic_in( 1 - x );
    }

    static float cubic_inout( float x )
    {
      if( x < 0.5 )
      {
        return cubic_in( x * 2 ) * 0.5;
      }
      else
      {
        return cubic_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float quartic_in( float x )
    {
      float y = x * x;
      return y * y;
    }

    static float quartic_out( float x )
    {
      return 1 - quartic_in( 1 - x );
    }

    static float quartic_inout( float x )
    {
      if( x < 0.5 )
      {
        return quartic_in( x * 2 ) * 0.5;
      }
      else
      {
        return quartic_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float quintic_in( float x )
    {
      float y = x * x;
      return y * y * x;
    }

    static float quintic_out( float x )
    {
      return 1 - quintic_in( 1 - x );
    }

    static float quintic_inout( float x )
    {
      if( x < 0.5 )
      {
        return quintic_in( x * 2 ) * 0.5;
      }
      else
      {
        return quintic_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float sinusoidal_in( float x )
    {
      return sinf( x * 0.5 * pi - pi * 0.5 ) + 1;
    }

    static float sinusoidal_out( float x )
    {
      return 1 - sinusoidal_in( 1 - x );
    }

    static float sinusoidal_inout( float x )
    {
      return sinf( x * pi - pi * 0.5 ) * 0.5 + 0.5;
    }

    static float exponential_in( float x )
    {
      float base = 1024;
      return x < 0.001 ? 0 : powf( base, x-1 );
    }

    static float exponential_out( float x )
    {
      return 1 - exponential_in( 1 - x );
    }

    static float exponential_inout( float x )
    {
      if( x < 0.5 )
      {
        return exponential_in( x * 2 ) * 0.5;
      }
      else
      {
        return exponential_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float circular_in( float x )
    {
      return 1 - sqrtf( 1 - x * x );
    }

    static float circular_out( float x )
    {
      return 1 - circular_in( 1 - x );
    }

    static float circular_inout( float x )
    {
      if( x < 0.5 )
      {
        return circular_in( x * 2 ) * 0.5;
      }
      else
      {
        return circular_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float elastic_in( float x )
    {
      float s, a = 0.1, p = 0.4;

      if( x < 0.001 ) return 0;
      if( x > 0.999 ) return 1;

      if( a < 1 )
      {
        a = 1; 
        s = p * 0.25;
      }
      else
      {
        s = p * asin( 1 / a ) / ( 2 * pi );
      }

      return a * powf( 2, (x - 1) * 10 ) * sinf( ( x - s ) * 2 * pi / p );
    }

    static float elastic_out( float x )
    {
      return 1 - elastic_in( 1 - x );
    }

    static float elastic_inout( float x )
    {
      if( x < 0.5 )
      {
        return elastic_in( x * 2 ) * 0.5;
      }
      else
      {
        return elastic_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float back_in( float x )
    {
      float s = 1.70158;
      return x * x * ( ( s + 1 ) * x - s );
    }

    static float back_out( float x )
    {
      return 1 - back_in( 1 - x );
    }

    static float back_inout( float x )
    {
      if( x < 0.5 )
      {
        return back_in( x * 2 ) * 0.5;
      }
      else
      {
        return back_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }

    static float bounce_in( float x )
    {
      x = 1 - x;
      float res = 0;

      if( x < ( 1 / 2.75 ) )
      {
        res = 7.5625 * x * x;
      }
      else if( x < ( 2 / 2.75 ) )
      {
        x = x - ( 1.5 / 2.75 );
        res = 7.5625 * x * x + 0.75;
      }
      else if( x < ( 2.5 / 2.75 ) )
      {
        x = x - ( 2.25 / 2.75 );
        res = 7.5625 * x * x + 0.9375;
      }
      else
      {
        x = x - ( 2.625 / 2.75 );
        res = 7.5625 * x * x + 0.984375;
      }

      return 1 - res;
    }

    static float bounce_out( float x )
    {
      return 1 - bounce_in( 1 - x );
    }

    static float bounce_inout( float x )
    {
      if( x < 0.5 )
      {
        return bounce_in( x * 2 ) * 0.5;
      }
      else
      {
        return bounce_out( (x - 0.5) * 2 ) * 0.5 + 0.5;
      }
    }
};