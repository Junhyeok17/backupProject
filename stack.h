// 파일 크기 한 자리씩 저장하기 위한 스택
typedef struct Stack{
	int stack[20];
	int top;
}Stack;

Stack stack;

// 스택 초기화
void stackInit()
{
	stack.top = -1;
}

// 스택에 숫자 저장
void stackAdd(int element)
{
	if(stack.top<20)
		stack.stack[++stack.top] = element;
	else{
		fprintf(stderr, "out of size error\n\n");
		exit(1);
	}
}

// 저장된 숫자 하나씩 빼기
int stackPop()
{
	if(stack.top>=0)
		return stack.stack[stack.top--];
	else{
		fprintf(stderr, "out of size error\n\n");
		exit(1);
	}
}

// 현재 스택에 저장된 숫자 개수 출력
int stackSize()
{
	return stack.top+1;
}
