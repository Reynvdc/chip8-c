
#include<time.h>
#include <stdio.h>



int main(void){

    float deltaTime;
    clock_t start,timePassed;
    start = clock();
    int seconds=0;
    printf("Starting of the program, start_t = %ld\n", start);
    
    printf("clocks per sec= %ld\n", CLOCKS_PER_SEC);

	while(1){
        timePassed = clock() - start;
        deltaTime =  (float) timePassed / CLOCKS_PER_SEC;
        if(deltaTime >= 1.0f){
            printf("Value deltatime= %f\n", deltaTime);
            printf("seconds past= %d\n", seconds++);
            deltaTime = 0.0f;
            start = clock();
        }
        
	}

}