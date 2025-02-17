class mySignal : public TObject{
    private:
        Long_t t0;
        Long_t TOT;
        Double_t aMax;
        Double_t Q;
    public:
        mySignal();
        mySignal(Long_t t0, Long_t TOT, Double_t aMax, Double_t Q);
        virtual ~mySignal();
        void set(Long_t t, Long_t T, Double_t A, Double_t Q1){
            t0 = t;
            TOT = T;
            aMax = A;
            Q = Q1;
        }
        void Print(Option_t *option = "") const;
        ClassDef(mySignal, 1);

};