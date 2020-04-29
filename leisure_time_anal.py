import csv
import numpy as np
import matplotlib.pyplot as plt

def read(csv_reader):
    """ Read data and preprocess 

    Args
        csv_reader

    Returns:
        res: a dict stores clockwise values sequences
    """
    res = {}
    for i,row in enumerate(csv_reader):
        if i == 0:
            fields = row
            for f in fields:
                res[f] = []
        else:
            for k,v in zip(fields, row):
                if i > 1 and k != "Time":
                    # real state at time res["Time"][-1]
                    res[k][-1] = v;
                res[k].append(v);

    for k, l in res.items(): # last redundant element
        res[k].pop()
        res[k] = np.array(res[k], dtype = int)
    
    return res

def pict(data):
    x = data["Time"]
    aux_xticks = []
    aux_yticks = []
    for lbl, var in data.items():
        if lbl != "Time":
            plt.plot(x, var, label = lbl)
            for i in range(len(x)-1, 0, -1):
                if var[i] != var[i-1]:
                    endx, endy = x[i], var[i]
                    plt.vlines(endx, 0, endy, linestyle="dashed", linewidth=0.3)
                    plt.hlines(endy, 0, endx, linestyle="dashed", linewidth=0.3)
                    aux_xticks.append(endx)
                    aux_yticks.append(endy)
                    break;

    _xticks = list(plt.xticks()[0])
    _xticks.append(max(aux_xticks))
    _xticks.remove(500)
    plt.xticks(_xticks)
    _yticks = (list(plt.yticks()[0])+ aux_yticks)
    _yticks.remove(300) # adjust
    plt.yticks(_yticks)

    plt.xlim(left=0)
    plt.ylim(bottom=0)
    plt.xlabel("Time")
    plt.ylabel('Count')
    plt.title("Scene Play: the Leisure Time")
    plt.legend()
    plt.show()


pth = "test_task_leisure_time.out"
with open(pth, "r", newline="") as csvfile:
    rd = csv.reader(csvfile)
    data = read(rd)
    pict(data)
