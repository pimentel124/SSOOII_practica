package comp;

import java.util.Scanner;

class Arbol {

    int x, y;

    public Arbol(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public boolean esVisible(int l, int maxx) {
        boolean vis = false;

        if (x >= (l - y)) {
            if (x <= (l + y)) {
                vis = true;
            }
        }

        return vis;
    }

}

public class Comp {

    public static void main(String[] args) {
        
        Scanner s = new Scanner(System.in);
        
        int aux = s.nextInt();
        int x;
        int y;
        
        while (aux != 0) {
            
        int miny = 200000,maxx = 0,arboles = 0,maxarb = 0;
       
            
            Arbol[] arb = new Arbol[aux];

            for (int i = aux; --i >= 0; ) {

                x = s.nextInt();
                y = s.nextInt();

                if (x > maxx) {
                    maxx = x;
                }
                if (y < miny) {
                    miny = y;
                }
                arb[i] = new Arbol(x, y);
            }

            for (int i = 1; i < maxx-miny; i++) {
                for (Arbol arb1 : arb) {
                    //Se comprueba la cantidad de árboles que se ven en la posicion i
                    if (arb1.esVisible(i, maxx)) {
                        arboles++;
                        if (arboles > maxarb) {
                        maxarb = arboles;
                    }
                    }
                    
                }
                arboles = 0;
            }
            aux = s.nextInt();
            System.out.println(maxarb);
        
        }
       
    }
}
