### Goal
Each router achieves connectivity via RIP, e.g. R1 pings R6.
### Topology
FRR's RIP running on R1, R2, R4, R5, R6 the tested RIP implementation on r3.

```

          10.0.1.0/24    10.0.2.0/24        10.0.3.0/24     10.0.4.0/24      10.0.5.0/24
+---------+      +---------+      +---------+      +---------+      +---------+      +---------+
|         |      |         |      |         |      |         |      |         |      |         |
|         |.1  .2|         |.2  .3|         |.3  .4|         |.4  .5|         |.5  .6|         |
|   R1    +------+   R2    +------+   R3    +------+   R4    +------+   R5    +------+   R6    |
|  (FRR)  |      |  (FRR)  |      |  RIPD   |      |  (FRR)  |      |  (FRR)  |      |  (FRR)  |
|         |      |         |      |         |      |         |      |         |      |         |
+---------+      +---------+      +---------+      +---------+      +---------+      +---------+
```
