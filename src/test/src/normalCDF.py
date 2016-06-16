"""
This code was copied and modified (for Python3) from:
http://www.johndcook.com/blog/python_phi_inverse/
"""

import math
 
def rational_approximation(t):
 
    # Abramowitz and Stegun formula 26.2.23.
    # The absolute value of the error should be less than 4.5 e-4.
    c = [2.515517, 0.802853, 0.010328]
    d = [1.432788, 0.189269, 0.001308]
    numerator = (c[2]*t + c[1])*t + c[0]
    denominator = ((d[2]*t + d[1])*t + d[0])*t + 1.0
    return t - numerator / denominator
 
 
def inverse(p):
 
    assert p > 0.0 and p < 1
 
    # See article above for explanation of this section.
    if p < 0.5:
        # F^-1(p) = - G^-1(p)
        return -rational_approximation( math.sqrt(-2.0*math.log(p)) )
    else:
        # F^-1(p) = G^-1(1-p)
        return rational_approximation( math.sqrt(-2.0*math.log(1.0-p)) )
 
def demo():
 
    print( "\nShow that the NormalCDFInverse function is accurate at" )
    print( "0.05, 0.15, 0.25, ..., 0.95 and at a few extreme values.\n\n" )
 
    p = [
        0.0000001,
        0.00001,
        0.001,
        0.05,
        0.15,
        0.25,
        0.35,
        0.45,
        0.55,
        0.65,
        0.75,
        0.85,
        0.95,
        0.999,
        0.99999,
        0.9999999
    ]
 
    # Exact values computed by Mathematica.
    exact = [
        -5.199337582187471,
        -4.264890793922602,
        -3.090232306167813,
        -1.6448536269514729,
        -1.0364333894937896,
        -0.6744897501960817,
        -0.38532046640756773,
        -0.12566134685507402,
         0.12566134685507402,
         0.38532046640756773,
         0.6744897501960817,
         1.0364333894937896,
         1.6448536269514729,
         3.090232306167813,
         4.264890793922602,
         5.199337582187471
    ]
 
    maxerror = 0.0
    num_values = len(p)
    print( "p, exact CDF inverse, computed CDF inverse, diff\n\n" );
     
    for i in range(num_values):
        computed = inverse(p[i])
        error = exact[i] - computed
        print( p[i], ",", exact[i], ",", computed, ",", error )
        if (abs(error) > maxerror):
            maxerror = abs(error)
 
    print( "\nMaximum error:" , maxerror , "\n" )
 
if __name__ == "__main__":
    demo()

