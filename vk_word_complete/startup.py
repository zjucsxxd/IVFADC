import os
import time

"""
fid = open("time_20000_1.txt", "w")
start_time = time.time()
fid.write(str(start_time)+'\n')
fid.close()

os.system("./ndk2 sample0.config 2")
os.system("./ndk2 sample0.config 3")

fid = open("time_20000_1.txt", "a")
end_time_1 = time.time()
fid.write(str(end_time_1)+'\n')
fid.close()

fid = open("time_8_5.txt", "a")
end_time_2 = time.time()
fid.write("index start: "+str(end_time_2)+'\n')
fid.close()

os.system("./ndk sample.config 4")

fid = open("time_8_5.txt", "a")
end_time_2 = time.time()
fid.write("index end: "+str(end_time_2)+'\n')
fid.close()
"""

for i in range(0,7):
    fid = open("time_8_5_"+str(i)+".txt", "a")
    end_time_2 = time.time()
    fid.write("search start: "+str(end_time_2)+'\n')
    fid.close()

    os.system("./ndk2 sample"+str(i)+".config 5 &")

    fid = open("time_8_5_"+str(i)+".txt", "a")
    end_time_3 = time.time()
    fid.write(str(end_time_3)+'\n')
    fid.close()

