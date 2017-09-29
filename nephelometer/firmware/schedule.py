def schedule(pct, count):
    p50 = (pct / 2) + ( 1 if ((pct % 2) > (count / 50)) else 0 )
    p25 = (p50 / 2) + ( 1 if ((p50 % 2) > ((count % 50) / 25)) else 0 )
    p5 = (p25 / 5) + ( 1 if ((p25 % 5) > ((count % 25) / 5)) else 0 )
    return (p5 > (count % 5))

for pct in xrange(0,100):
    fracton = 0
    line = ""
    for cycle in xrange(0,99):
        if schedule(pct, cycle):
            fracton = fracton + 1
            line = line + "#"
        else:
            line = line + " "
    print "{0:03}\t{1}".format(fracton, line)
