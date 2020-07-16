#include "hmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define max(a,b) a > b ? a : b; 

typedef struct {
	hmmType hmm; //hmm of each words(or unigrams)
	char* word; //word
	int phnum;	//number of phones
} word_hmms_type;

void make_unigram(int,word_hmms_type*,word_hmms_type*);	//make unigram for each words
float* make_bigram(int,word_hmms_type*,float*); //create bigrams transition matrix Tb, size = (wordcount X wordcount)
hmmType* ADD_HMM(hmmType*,int,int);	//add two hmms (state#=3)
hmmType* ADD_SP(hmmType*,int,int);	//add sp hmm (state#=1)
float calc_gausian(int,int,word_hmms_type*,int,float*); //calculate gausian probability
void Viterbi(char*,int,word_hmms_type*,word_hmms_type*,float*,float*); //Viterbi algorithm

int main(){
	FILE *dict;
	
	if(!(dict = fopen("dictionary.txt","r"))){
		printf("Error opening file\n");
		exit(1);
	}
	
	char line[35]; //dict의 각 line
	word_hmms_type word_hmms[15];
	int wordcount=0;

	while(fgets(line, sizeof(line), dict)){	//create word hmms
		
		if(line[strlen(line)-1]=='\n'){
			line[strlen(line)-1]='\0';
		}
		
		char* word_phones[10]={NULL,};	//save phone in word_phones array
		char* word;	//save word
		int i=-1;
		
		char * token;
		token = strtok (line, "' ''\t'"); //seperate each line with " " or "\t"
		
		while(token != NULL){
			if(i<0){ //save word
				word=token;
				i++;  
			}
			else {
				if(token[strlen(token)-1]==13) //carriage return
					token[strlen(token)-1]='\0';
				word_phones[i] = token; //save phone 
				i++; //count phone #
			}
			token = strtok(NULL,"' ''\t'");
		}//get word, word_phones
		int phone_count = i; //number of phones
		
	////combine hmms
		hmmType* new_hmm=(hmmType*)malloc(sizeof(hmmType)); 
		int front_hmm_index;
		int back_hmm_index;	//index of hmm to be combined
				
		for(int j=0;j<21;j++){ //find first phone hmm as front_hmm
			if(strcmp(word_phones[0],phones[j].name)==0){ 
				(*new_hmm) = phones[j];
				break;
			}
		}
		
		int k=1;
		int flag=1;
		while(k < phone_count){ //adding phones one by one using ADD_HMM(hmm index,hmm index)
			for(i=0;i<21;i++){
				if((strcmp(word,"zero")==0)&&(k==4)&&flag){ //zero에 iy state 추가
					hmmType* temp_hmm = ADD_HMM(new_hmm,15,k);
					(*new_hmm) = (*temp_hmm);
					flag=0; k--;
					break;
				}
				if(strcmp(word_phones[k],"sp")==0){//add sp
					if(flag==0) k=k+1; //iy가 추가된 경우
					hmmType* temp_hmm = ADD_SP(new_hmm,17,k);
					(*new_hmm) = (*temp_hmm);
					break;
				}
				if(strcmp(word_phones[k],phones[i].name)==0){
					back_hmm_index = i;	//find phone to add next to front_hmm
					hmmType* temp_hmm = ADD_HMM(new_hmm,back_hmm_index,k);
					(*new_hmm) = (*temp_hmm);
					break;
				}
			}
			k++;
		}
		//save created word hmm into word_hmms array
		(word_hmms[wordcount].hmm)=(*new_hmm);
		(word_hmms[wordcount].word)=strdup(word);
		word_hmms[wordcount].phnum = phone_count;
		
		wordcount++;
	}
	fclose(dict);
	
	wordcount-=1; //zero combined
	//word_hmms[wordcount] 배열에 저장되어있는 여러 개의 hmm을 결합한다
	printf("# of words = %d\n",wordcount); //wordcount=12

	//change trans matrix of zero a bit (connect z->iy, iy->r)
	(word_hmms[11].hmm).tp[3][13] = phones[8].tp[3][4] * phones[15].tp[0][1]; //z->iy
	(word_hmms[11].hmm).tp[6][16] = phones[15].tp[3][4] * phones[3].tp[0][1]; //iy->r
	(word_hmms[11].hmm).tp[12][16] = phones[16].tp[3][4] * phones[17].tp[0][1]; //ow->sp
	(word_hmms[11].hmm).tp[15][16] = 0.000000e+000; //disconnect iy->sp

	//word hmm tp 검증
	printf("ex: zero word's hmm => Tp\n");
	for(int i=0;i<18;i++){
		for(int j=0;j<18;j++){
			printf("%.3f | ",(word_hmms[11].hmm).tp[i][j]);	//zero1
		}
		printf("\n");
	}
	printf("\n");
	
	/////////create unigrams (read unigram.txt)
	word_hmms_type uni_hmms[15];
	make_unigram(wordcount,word_hmms,uni_hmms);	//make unigram hmm
	//Unigram검증
	printf("Unigram Reflected:\n");
	for(int i=0;i<wordcount;i++){
		printf("word=%s, unigram name=%s,starting_prob=%f,#of phones=%d\n",uni_hmms[i].word, (uni_hmms[i].hmm).name,(uni_hmms[i].hmm).tp[0][1],uni_hmms[i].phnum);
	}
	printf("\n");
	
	float* Tb = (float*)malloc(sizeof(float)*(wordcount*wordcount));
	Tb = make_bigram(wordcount,word_hmms,Tb); //make trans matrix between word hmms (bigram)
	//Tb검증
	int x,y;
	printf("Trans Matrkx between word hmms => Tb:\n");
	for(int i=0;i<12*12;i++){
		x=(i-1)/12; y=i-x;
		printf("%.4f| ",Tb[i]);
		if(i%12==11) printf("\n");
	}
	printf("\n");
	
	char* tempfilename="tst/f/ak/1237743.txt"; //file name
	float* features = (float*)malloc(sizeof(float)*(N_DIMENSION)); //feature vector

	Viterbi(tempfilename,wordcount,word_hmms,uni_hmms,Tb,features); //Viterbi Algorithm
	
	return 0;	
}

hmmType* ADD_HMM(hmmType* f,int b,int k){ //kth addition (f+b)
	hmmType* front = f;
	hmmType back = phones[b];
	int n_state=N_STATE*(k+1); //new number of states 

	//new name concatenation
	char* front_name = strdup(front->name);
	char* back_name = strdup(back.name);
	strcat(front_name, back_name); 
	front->name=NULL;
	(front->name) = strdup(front_name);
	free(front_name); 	free(back_name);
	
	//new state concatenation
	for(int i=0;i<N_STATE;i++){ 
		(front->state)[i+(n_state-2)-1] = back.state[i];
	}
	//new transition matrix concatenation 
	for(int i=0;i<4;i++){
		for(int j=1;j<5;j++){
			if(i==0 && j==1)
				(front->tp)[i+N_STATE*k][j+N_STATE*k] *= back.tp[i][j];
			else{
				(front->tp)[i+N_STATE*k][j+N_STATE*k] = back.tp[i][j];
			}
		}
	}
	return front;	
}

hmmType* ADD_SP(hmmType* f,int b,int k){ //kth addition (f+b)
	hmmType* front = f;
	hmmType back = phones[b];
	int n_state=N_STATE*(k)+1; //new number of states //sp는 state 1개 추가

	//new name concatenation
	char* front_name = strdup(front->name);
	char* back_name = strdup(back.name);
	strcat(front_name, back_name); 
	front->name=NULL;
	(front->name) = strdup(front_name);
	free(front_name); 	free(back_name);
	
	//new state concatenation
	(front->state)[n_state-1] = back.state[0];
	
	//new transition matrix concatenation
	for(int i=0;i<2;i++){
		for(int j=1;j<3;j++){
			if(i==0 && j==1)
				(front->tp)[i+(n_state-1)][j+(n_state-1)] *= back.tp[i][j];
			else{
				(front->tp)[i+(n_state-1)][j+(n_state-1)] = back.tp[i][j];
			}
		}
	}
	return front;	
}

void make_unigram(int wordcount,word_hmms_type* word_hmms,word_hmms_type* uni_hmms){
	FILE *uni;
	//word_hmms_type uni_hmms[15];
	for(int i=0;i<wordcount;i++){
		uni_hmms[i] = word_hmms[i];
	}
	
	if(!(uni = fopen("unigram.txt","r"))){
		printf("Error opening file\n");
		exit(1);
	}
	
	char line[35]; //dict의 각 line
	while(fgets(line, sizeof(line), uni)){
		if(line[strlen(line)-1]=='\n'){
			line[strlen(line)-1]='\0';
		}

		char* word1;
		float prob;
		char * token;
		
		token = strtok (line, "\t"); //word1
		word1=token;

		token = strtok(NULL,"\t");  //prob
		if(token[strlen(token)-1]==13) //carriage return
			token[strlen(token)-1]='\0';
		prob = atof(token); 
		
		for(int i=0;i<wordcount;i++){
			if(strcmp(uni_hmms[i].word, word1)==0){
				((uni_hmms[i].hmm).tp[0][1]) *= prob;
				break;
			}
		}//for
	}//while
	fclose(uni);
}

//create bigrams transition matrix Tb sized wordcount * wordcount
float* make_bigram(int wordcount,word_hmms_type* word_hmms,float* Tb){
	FILE *bi;
	if(!(bi = fopen("bigram.txt","r"))){
		printf("Error opening file\n");
		exit(1);
	}
	char line[35]; //dict의 각 line
	while(fgets(line, sizeof(line), bi)){
		if(line[strlen(line)-1]=='\n'){
			line[strlen(line)-1]='\0';
		}
		char* token; char* word1; char* word2;
		float prob;	int x,y;
		
		token = strtok (line, " \t"); 
		word1=token;	//word1
		token = strtok(NULL," \t");  
		if(token[strlen(token)-1]==13) //carriage return
			token[strlen(token)-1]='\0';
		word2=token;	//word2
		
		token = strtok(NULL," \t");
		if(token[strlen(token)-1]==13) //carriage return
			token[strlen(token)-1]='\0';
		prob = atof(token); //prob
		
		//find word1 index
		for(int i=0;i<wordcount;i++){
			if(strcmp(word_hmms[i].word, word1)==0){
				x=i; 
				break;
			}
		}//for
		//find word2 index
		for(int i=0;i<wordcount;i++){
			if(strcmp(word_hmms[i].word, word2)==0){
				y=i; 
				break;
			}
		}//for
		Tb[x*wordcount+y] = prob;	//push prob into table Tb
	}//while
	fclose(bi);
	return Tb;
}

/*
//////////////////////////////////////////////////////////////////
////************** Viterbi Algorithm for HMM *****************////
//////////////////////////////////////////////////////////////////
	
void Viterbi(char* filename,int wordcount,word_hmms_type* word_hmms,word_hmms_type* uni_hmms,float* Tb,float* features){
	FILE* feat;

	if(!(feat = fopen(filename,"r"))){
		printf("Error opening file\n");
		exit(1);
	}	
	char line[2000]; //each line
	int countline=0;
	int dimension,nrows;
	
	fgets(line, sizeof(line), feat);
	//initial line read
	if(line[strlen(line)-1]=='\n'){
			line[strlen(line)-1]='\0';
		}
	char * token;
	token = strtok (line, " \t");
	nrows=atoi(token);
	
	token = strtok(NULL," \t");
	dimension = atoi(token);

	float m1[nrows][wordcount]; 
	int m2[nrows][wordcount];
	
	//next line read
	int k=1;	
	while(fgets(line, sizeof(line), feat)){	//each 39 dimension feature vector
		if(line[strlen(line)-1]=='\n'){
			line[strlen(line)-1]='\0';
		}
		
		token = strtok (line, " ");
		int countfeature=0;
	
		while(token != NULL){
			features[countfeature++]=atof(token);	//feature vector -> put into array
			token = strtok(NULL,"  ");
		}
	//////////////////viterbi algorithm start//////////////////////////
	
	//for all unigrams (initialize at time k=1)
	if(k==1){
		for(int j=0;(j<wordcount)&&(k==1);j++){
			m1[1][j]=(uni_hmms[j].hmm).tp[0][1] * calc_gausian(0,j,uni_hmms,dimension,features);
			m2[1][j]=0;
			k++;
		}
		continue;
	}
	//fgets의 각 line 하나하나에 해당. 39차원 벡터 하나에 대해 아래를 검사
		
		for(int j=0;j<wordcount;j++){
			//bigram transition matrix Tb (i->j)
			float p[(word_hmms[j].phnum-1)*3+1+1][(word_hmms[j].phnum-1)*3+1+1];
			for(int s=1;s<=(word_hmms[j].phnum-1)*3+1;s++){ //s-1은 state index, state#=s
				p[1][s]=(word_hmms[j].hmm).tp[0][s];
			}

			float gausian=-100; //gausian 확률값
			float calc_temp;
			for(int t=1;t<=(word_hmms[j].phnum-1)*3+1;t++){ //진행 횟수 t에서
				float temp=-100;
				for(int s=1;s<=(word_hmms[j].phnum-1)*3+1;s++){ //각 s에 대해
					for(int v=1;v<=(word_hmms[j].phnum-1)*3+1;v++){ //모든 가능한 transition v->s에 대해 max값 저장
						if(((word_hmms[j].hmm).tp[v][s])!=0){
							calc_temp=p[t-1][v]*((word_hmms[j].hmm).tp[v][s])*calc_gausian(s-1,j,word_hmms,dimension,features);
							p[t][s]=max(p[t][s],calc_temp);
						}
					}//v
				}//s	
				gausian = max(gausian,p[t][(word_hmms[j].phnum-1)*3+1]); //모든 t에 대해 가장 큰 output 값r구함
			}//t

			//update m1[k][j], m2[k][j]
			float temp1;
			for(int i=0;i<wordcount;i++){
				temp1 = m1[k][j];
				m1[k][j]=max(temp1,m1[k-1][j] * Tb[i*wordcount+j] * gausian);
				if(temp1 != m1[k][j])
					m2[k][j]=i;
			}
		}//j
		k++;
	////////////////viterbi algorithm end//////////////////////////////
	}	
	fclose(feat);
	
//////print result into file
	//FILE* res;
	//res = fopen("recognized.txt","w");
	
	int result[nrows];
	int temp1=-10; int temp2=-10;
	for(int i=0;i<wordcount;i++){
		temp2=max(temp1,m1[nrows][i]);
		if(temp1 != temp2) //index update condition
			result[nrows]=i;
		temp1=temp2;
	}
	//fprintf(res,"#!MLF!#\n");
	//fprintf(res,"\"%s.rec\"\n",filename);
	printf("Final Result:\n");
	printf("%s\n",word_hmms[result[nrows]].word);
	
	for(int c=nrows-1;c>nrows-8;c--){
		result[c]= m2[c][result[c+1]];
		printf("%s\n",word_hmms[result[c]].word);
		//fprintf(res,"%s\n",word_hmms[result[c]].word);
	}
	//fprintf(res,".\n");
	//fclose(res);
}


float calc_gausian(int s_index,int j,word_hmms_type* word_hmms,int dimension,float* features){
	//각 gausian은 word_hmms[j].state[1부터 (word_hmms[j].phnum-1)*3+1까지].pdf[0 아니면 1]에 저장돼있다.
	// j = word index
	float pi_val = pow(2*M_PI,dimension/2);
	
	//gausian model 1의 값 구하기 (p_gaus1)
		pdfType pdf1;
		pdf1= (word_hmms[j].hmm).state[s_index].pdf[0];
		float weight1 = pdf1.weight; //float		
		//with log
		float log_sum_var1=0;
		float p_gaus1=0;
		for(int i=0;i<dimension;i++){
			log_sum_var1+=1/2*logf(pdf1.var[i]);	
			p_gaus1+= pow((features[i] - pdf1.mean[i]),2)/(pdf1.var[i]);
		}
		float log_p_gaus1 = logf(weight1)-(dimension/2)*logf(2*M_PI)-log_sum_var1- (1/2)*p_gaus1;
		
	//gausian model 2의 값 구하기 (p_gaus2)
		pdfType pdf2;
		pdf2= (word_hmms[j].hmm).state[s_index].pdf[1];
		float weight2 = pdf2.weight; //float
		
		//with log
		float log_sum_var2=0;
		float p_gaus2=0;
		for(int i=0;i<dimension;i++){
			log_sum_var2+=1/2*logf(pdf2.var[i]);	
			p_gaus2+= pow((features[i] - pdf2.mean[i])/pdf2.var[i] , 2);
		}
		float log_p_gaus2 = logf(weight2)-(dimension/2)*logf(2*M_PI)-log_sum_var2- (1/2)*p_gaus2;
		
	//결과 합치기
		float result = log_p_gaus1 + logf(1+exp(log_p_gaus2-log_p_gaus1));

	return result;
}
