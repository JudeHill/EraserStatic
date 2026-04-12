int main(void){
    return recursive(10);
}

int recursive(int n){
    if (n == 0){
        return n;
    }
    return recursive(n-1);
}