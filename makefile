/usr/bin/g++ main.cpp common.h mycurl.* -I ./curl  -lz -lstdc++ -lrt `mysql_config --cflags --libs`  /usr/local/lib/libconfig.a /home/chenyu/pcap_server/lib/libcurl.a -ljsoncpp -g -o data_analyser
