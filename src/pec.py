'''
Functions -
ranges_to_points(dim_ranges): given ranges for a single dimension, returns the list of points that must be tested for PECs.
identify_pec(ranges, point): given ranges for all dimensions and a specific point, returns the tuple that represents the PEC the point belongs to.
pec_calc(ranges): given ranges for all dimensions, return labels for distinct PECs.

'''

import itertools
'''
params------------
dim_ranges : list of tuples, where each tuple represents a range in a specific domain. ex) [(10,20), (15,25)] if we have two invariants,
            first inv ranges from 10 to 20, second inv ranges from 15 to 25 for the dimension of interest.

ret---------------
pl : list of points that must be evaluated for this dimension
'''


def ranges_to_points(dim_ranges):
    tl = []
    pl = set()
    for r in dim_ranges:
        pl.add(r[0])
        pl.add(r[1] + 1)
    pl = list(pl)
    pl.sort()
    pl = pl[:-1]
    #pl=set(pl)
    return pl


'''
params------------
ranges : list of lists of tuples, where each tuple represents a range for a specific dimension, and each inner list represents an invariant. ex) [[(10,20), (10,20)],[(15,25),(15,25)]]
        if we have two invariants for 2d space, first inv ranges from 10 to 20 for both dimensions, second inv ranges from 15 to 25 for both dimensions.
point : A tuple that represents a point in the whole domain. ex) (12, 13) for 2d

ret---------------
tuple(ret): a tuple that represents the PEC that the point belongs to. ex) (0,2,3) if the point belongs to the overlap of the first, third, and fourth invariants.
'''


def identify_pec(ranges, point):
    ret = []
    for j, r in enumerate(ranges):
        f = 0
        for i, pd in enumerate(point):
            if r[i][0] > pd or r[i][1] < pd:
                f = 1
        if f == 0:
            ret.append(j)

        ret.sort()
    return tuple(ret)


'''
params------------
ranges: same as ranges for identify_pec.

ret---------------
pecs : list of tuples, each representing distinct overlaps for the given ranges.
'''


def pec_calc(ranges):
    points = []
    pecs = set()
    for dim in range(len(ranges[0])):
        dim_range = [r[dim] for r in ranges]
        points.append(ranges_to_points(dim_range))

    points = itertools.product(*points)

    for p in points:
        pec = identify_pec(ranges, p)
        if pec != ():
            pecs.add(pec)
    return pecs


'''

PEC=identify_PEC([[(10,20),(10,20)],[(15,25),(15,25)]], (11,11))
print(PEC)

points=ranges_to_points([(10,16),(12,18)])
print(points)


#ret=pec_calc([[(10,20),(10,20),(10,20)],[(15,25),(15,25),(15,25)],[(11,23),(17,30),(10,30)]])
'''
ret = pec_calc([[(10, 20), (10, 20), (10, 20)], [(10, 20), (10, 20), (10, 20)],
                [(10, 20), (10, 20), (10, 20)]])
print(ret)
'''
#This function is not used.

def get_intersection(a_ranges, b_ranges):
    ranges=[]
    for i in range(len(a_ranges)):
        start=max(a_ranges[i][0], b_ranges[i][0])
        end=min(a_ranges[i][1], b_ranges[i][1])

        if start<=end:
            ranges.append((start, end))
        else:
            return None
    return ranges
'''
