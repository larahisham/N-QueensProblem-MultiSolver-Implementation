import matplotlib.pyplot
import pandas
#insert your path here 
DFS_q = pandas.read_csv()
LOCAL_q = pandas.read_csv()
CSP_q = pandas.read_csv()
matplotlib.pyplot.plot(DFS_q["N"], DFS_q["Time(seconds)"], marker='o', label="Blind DFS (C++)",color = 'purple')
matplotlib.pyplot.plot(LOCAL_q["N"], LOCAL_q["Time(seconds)"], marker='o', label="Local search (C++)",color = 'blue')
matplotlib.pyplot.plot(CSP_q["N"], CSP_q["Time(seconds)"], marker='x', label="CSP with MRV+LCV+FC (C++)",color = 'green')
# matplotlib.pyplot.xlabel("N (Board Size)")
# matplotlib.pyplot.ylabel("Time (seconds)")
# matplotlib.pyplot.title("Performance of Blind DFS vs CSP (N-Queens, C++)")
# matplotlib.pyplot.legend()
# matplotlib.pyplot.grid(True)
# matplotlib.pyplot.tight_layout()
# matplotlib.pyplot.show()


# zoomed in 

matplotlib.pyplot.xlabel("N (Board Size)")
matplotlib.pyplot.ylabel("Time (seconds)")
matplotlib.pyplot.title("Performance of Blind DFS vs CSP (N-Queens, C++)")
matplotlib.pyplot.yscale('log')
matplotlib.pyplot.legend()
matplotlib.pyplot.grid(True, which = "both", linestyle='--', linewidth=0.5)
matplotlib.pyplot.tight_layout()
matplotlib.pyplot.show()