int main() {
    int value = 128;
    if (value % 2 == 0){
        if (value % 5 == 0){
            printf("a");
        } else {
            printf("b");
        }
    } else {
        if (value % 6 == 0){
            printf("c");
        } else {
            printf("d");
        }
    }
}