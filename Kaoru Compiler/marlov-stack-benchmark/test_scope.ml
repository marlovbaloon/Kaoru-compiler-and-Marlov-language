@func marlov_test_scope() {
    @int a = 1;
    {
        @int b = 2;
        if (a == b) {
            @int c = 3;
            return;
        }
    }
}