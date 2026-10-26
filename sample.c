#include<stdio.h>
#if 0
int main()
{

    int a = 5;
    int b = 8;
    if(a>b)
    {
        printf("true");
    }
    else if(a==b)
    {
        printf("false");
    }else if(a!=b)
    {
        printf("nothing");
    }else
    {
        printf("wild");
    }
         return 0;
}

#endif

#if 0
int main()
 {

    int num;
    printf("Enetr the number:");
    scanf("%d",&num);

    if(num>0)
    {
        printf("Positive number\n");
    }
    else if(num<0)
    {
        printf("Negative number\n");
    }
    else
    {
        printf("Number is 0\n");
    }
    return 0;
}
#endif

#if 0
/*
find the middle number
Enter the Three numbers : 10 20 30

The middle number is 20
*/
int main()
{
    int num1,num2,num3,middle;
    printf("Enter three numbers:\n");
    scanf("%d %d %d",&num1,&num2,&num3);

    if((num1 > num2 && num1 < num3) || (num1 < num3 && num1 > num2))
    {
        printf("The number is %d\n",num1);
    }
    else if((num2 > num1 && num2 < num3) || (num2 < num3 && num2 > num1))
    {
        printf("The number is %d\n",num2);
    }
    else
    //if((num2 > num1 && num2 < num3) || (num2 < num3 && num2 > num1))
    {
        printf("The number is %d\n",num3);
    }
    return 0;
}
#endif
