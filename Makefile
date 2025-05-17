default:
	gcc -o main main.c -l wiringPi
full:
	git pull origin main && gcc -o main main.c -lwiringPi -lm -li2c && ./main
