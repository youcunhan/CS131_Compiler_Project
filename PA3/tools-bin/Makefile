compare:
	./lexer good.cl | ./parser | ./semant_right > _correct_answer.out
	./lexer good.cl | ./parser | ./semant > _your_answer.out
	diff _correct_answer.out _your_answer.out
you:
	./lexer good.cl | ./parser | ./semant > _your_answer.out
corr:
	./lexer good.cl | ./parser | ./semant_right > _correct_answer.out