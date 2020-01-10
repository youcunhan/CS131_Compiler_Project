class C {
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C {
           {
		a <- x;
		b <- y;
		self;
           }
	};
};


class A inherits Int{
	c : Int;
	d : Bool;
	init1(x : Int, y : Bool) : A {
           {
		c <- x;
		d <- y;
		self;
           }
	};
};
class B inherits C {
	c : Int;
	d : Bool;
	init2(x : Int, y : Bool) : B {
           {
		c <- x;
		d <- y;
		self;
           }
	};
};
Class Main {
	main():C{
	  (new C).init(1,true)
	};
};
