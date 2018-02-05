from subprocess import Popen, PIPE
from matplotlib import pyplot as plt

FILE_NAME = r'C:\Users\Naganithin\Documents\oslab\3\Ass3a_51.exe'

def run(num):
    ans = [0, 0, 0, 0, 0]
    for _ in range(10):
        prog = Popen([FILE_NAME, str(num)], stdout=PIPE)
        pipeout = prog.communicate()[0].split(b'\n')
        for i in range(5):
            ans[i] += float(pipeout[i])
    for i in range(5):
        ans[i] /= 10
    return ans

if __name__ == '__main__':
    r10 = run(10)
    r50 = run(50)
    r100 = run(100)
    fcfs = [r10[0], r50[0], r100[0]]
    sjf = [r10[1], r50[1], r100[1]]
    rr1 = [r10[2], r50[2], r100[2]]
    rr2 = [r10[3], r50[3], r100[3]]
    rr5 = [r10[4], r50[4], r100[4]]
    plt.plot([10, 50, 100], fcfs)
    plt.plot([10, 50, 100], sjf)
    plt.plot([10, 50, 100], rr1)
    plt.plot([10, 50, 100], rr2)
    plt.plot([10, 50, 100], rr5)
    plt.legend(['FCFS', 'SJF', 'RR with 1', 'RR with 2', 'RR with 5'])
    plt.xlabel('N')
    plt.ylabel('ATN')
    plt.show()
