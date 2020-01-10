

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <set>
#include <list>
#include "semant.h"
#include "utilities.h"


extern int semant_debug;
extern char *curr_filename;

static Class_ curr_class = NULL;
static ClassTable* classtable;
static SymbolTable<Symbol, Symbol> attribtable;
static SymbolTable<Symbol, Symbol> *attribtable_init;

typedef SymbolTable<Symbol, method_class> MethodTable;
static std::map<Symbol, MethodTable> methodtables;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}


/*
constructor of ClassTable ,which construct a class table for checking.
It can detect class semantic errors:
    (1)Redefinition of basic class.
    (2)Redefinition of previously defined class.
    (3)Class inherits Int || Str || SELF_TYPE || Bool .
    (4)Class inherits from an undefined class.
    (5)It contains cyclic inheritance.
    (6)Class Main is not defined.
**Contributor: youch
*/
ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    install_basic_classes();
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {

        // class name cannot be SELF_TYPE
        if (classes->nth(i)->GetName() == SELF_TYPE||classes->nth(i)->GetName() == Int||classes->nth(i)->GetName() == Bool||classes->nth(i)->GetName() == IO||classes->nth(i)->GetName() == Str||classes->nth(i)->GetName() == Object) {
            semant_error(classes->nth(i)) << "Redefinition of basic class " << classes->nth(i)->GetName() << "." << std::endl;
            return;
        }

        // class cannot be declared before
        if (m_classes.find(classes->nth(i)->GetName()) == m_classes.end()) {
            m_classes.insert(std::make_pair(classes->nth(i)->GetName(), classes->nth(i)));
        } else {
            semant_error(classes->nth(i)) << "Class " << classes->nth(i)->GetName() << " was previously defined." << std::endl;
            return;
        }

    }

    // Check the inheritance one by one.
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {

        curr_class = classes->nth(i);

        Symbol parent_name = curr_class->GetParent();
        while (parent_name != Object && parent_name != classes->nth(i)->GetName()) {

            // check that the parent is not Int or Bool or Str or SELF_TYPE
            if (parent_name == Int || parent_name == Str || parent_name == SELF_TYPE || parent_name == Bool) {
                semant_error(curr_class) << "Class " << curr_class->GetName() << " cannot inherit class " << parent_name << "." << std::endl;
                return;
            }

            // check that the parent of curr_class is present in m_classes
            if (m_classes.find(parent_name) == m_classes.end()) {
                semant_error(curr_class) << "Class "<< curr_class->GetName() <<" inherits from an undefined class " << parent_name << "." << std::endl;
                return;
            }

            curr_class = m_classes[parent_name];
            parent_name = curr_class->GetParent();

        }

        if (parent_name != Object) {
            semant_error(curr_class) << "Class "<<curr_class->GetName()<<", or an ancestor of "<<curr_class->GetName()<<", is involved in an inheritance cycle." << std::endl;
            return;
        }

    }

    // Check if we can find class Main.
    if (m_classes.find(Main) == m_classes.end()) {
        semant_error() << "Class Main is not defined." << std::endl;
    }

}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    m_classes.insert(std::make_pair(Object, Object_class));
    m_classes.insert(std::make_pair(IO, IO_class));
    m_classes.insert(std::make_pair(Int, Int_class));
    m_classes.insert(std::make_pair(Bool, Bool_class));
    m_classes.insert(std::make_pair(Str, Str_class));
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 



/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* construct the class table, detect class semantic errors */
    classtable = new ClassTable(classes);

    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    /* construct the method table, detect method semantic errors in one class*/
    construct_methodtables();
    checkinheritedmethods();


    /* construct the attribute table, explore and decorate the AST*/
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        curr_class = classes->nth(i);
        attribtable_init=new SymbolTable<Symbol, Symbol>;
        attribtable_init->enterscope();

        std::list<Symbol> path = classtable->GetAllParents(curr_class->GetName());
        for (std::list<Symbol>::iterator iter = path.begin(); iter != path.end(); iter++) {
            Class_ the_class = classtable->m_classes[*iter];
            Features curr_features = the_class->GetFeatures();
            attribtable.enterscope();
            for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
                Feature curr_feature = curr_features->nth(j);
                if(curr_feature->isattribute()) curr_feature->AddToAttributeTable(the_class->GetName());
                if(curr_feature->isattribute()&&the_class->GetName()!=curr_class->GetName()) attribtable_init->addid(curr_feature->GetName(), new Symbol(curr_feature->GetType()));
            }
        }
        
        curr_class = classes->nth(i);
        Features curr_features = curr_class->GetFeatures();

        for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
            Feature curr_feature = curr_features->nth(j);
            curr_feature->Explore();
        }

        for (int j = 0; j < path.size(); ++j) {
            attribtable.exitscope();
        }
    }
    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}

/* construct the method table, detect method semantic errors in one class
contributor: youch
*/
void program_class::construct_methodtables(){
    for (std::map<Symbol, Class_>::iterator iter = classtable->m_classes.begin(); iter != classtable->m_classes.end(); ++iter) {
        Symbol name = iter->first;
        methodtables[name].enterscope();
        Features curr_features = classtable->m_classes[name]->GetFeatures();
        for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
             Feature curr_feature = curr_features->nth(j);
             if(curr_feature->ismethod()){
                curr_feature->AddToMethodTable(name);
             }
        }
    }
}


void program_class::checkinheritedmethods(){
    for (std::map<Symbol, Class_>::iterator iter = classtable->m_classes.begin(); iter != classtable->m_classes.end(); ++iter) {
        
        Symbol class_name = iter->first;
        curr_class = classtable->m_classes[class_name];

        Features curr_features = classtable->m_classes[class_name]->GetFeatures();

        for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
            
            Feature curr_method = curr_features->nth(j);

            if (curr_method->isattribute()) {
                continue;
            }

            Formals curr_formals = ((method_class*)(curr_method))->GetFormals();
            
            std::list<Symbol> path = classtable->GetAllParents(class_name);
            for (std::list<Symbol>::reverse_iterator iter = path.rbegin(); iter != path.rend(); ++iter) {
                
                Symbol ancestor_name = *iter;
                method_class* method = methodtables[ancestor_name].lookup(curr_method->GetName());
                
                if (method != NULL) {
                    Formals formals = method->GetFormals();
                    if(curr_method->GetType()!=method->GetType()){
                        classtable->semant_error(curr_class->get_filename(),curr_method) << "In redefined method "<<curr_method->GetName()<<", return type "<<curr_method->GetType()<<" is different from original return type "<<method->GetType()<<"." << std::endl;
                    }

                    int k1 = formals->first(), k2 = curr_formals->first();
                    for (; formals->more(k1) && curr_formals->more(k2); k1 = formals->next(k1), k2 = curr_formals->next(k2)){}

                    if (formals->more(k1) || curr_formals->more(k2)) {
                        classtable->semant_error(curr_class->get_filename(),curr_method) << "Incompatible number of formal parameters in redefined method "<<curr_method->GetName()<<"." << std::endl;
                    }
                    k1 = formals->first();
                    k2 = curr_formals->first();
                    for (; formals->more(k1) && curr_formals->more(k2); k1 = formals->next(k1), k2 = curr_formals->next(k2)) {
                        if (formals->nth(k1)->GetType() != curr_formals->nth(k2)->GetType()) {
                            classtable->semant_error(curr_class->get_filename(),curr_formals->nth(k2)) << "In redefined method init, parameter type "<<curr_formals->nth(k2)->GetType()<<" is different from original type " << formals->nth(k1)->GetType() << std::endl;
                        }
                    }
                }
            }
        }
    }
}


/*add the current method to method table, can detect multiply-defined method in one class.
contributor: youch
*/
void method_class::AddToMethodTable(Symbol class_name) {
    if(methodtables[class_name].probe(this->GetName())==NULL){
        std::set<Symbol> formal_names;
        for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
            Symbol type = formals->nth(i)->GetType();
            Symbol formal_name = formals->nth(i)->GetName();
            if (formals->nth(i)->GetName() == self) {
                classtable->semant_error(curr_class->get_filename(),formals->nth(i)) << "'self' cannot be the name of a formal parameter." << std::endl;
            }
            if (type == SELF_TYPE) {
                classtable->semant_error(curr_class->get_filename(),formals->nth(i)) << "Formal parameter "<<formal_name<<" cannot have type SELF_TYPE." << std::endl;
            }
            if (classtable->m_classes.find(type) == classtable->m_classes.end() &&type != SELF_TYPE) {
                classtable->semant_error(curr_class->get_filename(),formals->nth(i)) << "Class "<<type<<" of formal parameter "<<formals->nth(i)->GetName()<<" is undefined." << std::endl;
                formals->nth(i)->SetType(Object);
            }
            if (formal_names.find(formal_name) != formal_names.end()) {
                classtable->semant_error(curr_class->get_filename(),formals->nth(i)) << "Formal parameter "<<formal_name<<" is multiply defined." << std::endl;
            } else {
                formal_names.insert(formal_name);
            }
        }
        methodtables[class_name].addid(name, new method_class(copy_Symbol(name), formals->copy_list(), copy_Symbol(return_type), expr->copy_Expression()));
    }
    else 
        classtable->semant_error(curr_class->get_filename(),this) << "Method "<<this->GetName()<<" is multiply defined." << std::endl;
}

/*add the current attribute to method table, can detect multiply-defined attribute in one class.
contributor: youch
*/
void attr_class::AddToAttributeTable(Symbol class_name) {
    if (name == self) {
        if(class_name==curr_class->GetName())
            classtable->semant_error(curr_class->get_filename(),this) << "'self' cannot be the name of an attribute."  << std::endl;
        return;
    }
    if (attribtable.lookup(name) != NULL ) {
        if(attribtable_init->probe(name)==NULL){
            classtable->semant_error(curr_class->get_filename(),this) << "Attribute " << name << " is multiply defined in class." << class_name <<curr_class->GetName()<<std::endl;
            return;
        }
        else
            classtable->semant_error(curr_class->get_filename(),this) << "Attribute " << name << " is an attribute of an inherited class." << std::endl;
            return;
    }

    attribtable.addid(name, new Symbol(type_decl));
}

/*get all classes inherited by the input class(including itself)
contributor:youch
*/
std::list<Symbol> ClassTable::GetAllParents(Symbol type) {
    if (type == SELF_TYPE) {
        type = curr_class->GetName();
    }

    std::list<Symbol> parents;

    for (; type != No_class; type = m_classes[type]->GetParent()) {
        parents.push_front(type); 
    }

    return parents;
}

bool ClassTable::IsSubclass(Symbol ancestor, Symbol child) {
    if (ancestor == SELF_TYPE) {
        return child == SELF_TYPE;
    }

    if (child == SELF_TYPE) {
        child = curr_class->GetName();
    }

    for (; child != No_class; child = m_classes.find(child)->second->GetParent()) {
        if (child == ancestor) {
            return true;
        }
    }
    return false;
}


/*explore each feature, calculate the type of every expreesion, check semantic errors, decorate the AST
contributor: youch
*/
void method_class::Explore() {
    attribtable.enterscope();
    std::set<Symbol> formal_names;
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        attribtable.addid(formals->nth(i)->GetName(), new Symbol(formals->nth(i)->GetType()));
    }

    if (classtable->m_classes.find(return_type) == classtable->m_classes.end() && return_type != SELF_TYPE) {
        classtable->semant_error(curr_class->get_filename(),this) << "Undefined return type " << return_type << " in method "<<name<<"." << std::endl;
    }
    
    Symbol expr_type = expr->Type();
    if (classtable->IsSubclass(return_type, expr_type) == false) {
        classtable->semant_error(curr_class->get_filename(),this) << "Inferred return type "<<expr_type<<" of method "<<name<<" does not conform to declared return type "<<return_type<<"." << std::endl;
    }
    attribtable.exitscope();
}
void attr_class::Explore() {
    Symbol rhs_type = init->Type();
    if (rhs_type != No_type && !classtable->IsSubclass(type_decl,rhs_type)) {
        classtable->semant_error(curr_class->get_filename(),this) << "Inferred type "<<rhs_type<<" of initialization of attribute "<<name<<" does not conform to declared type "<<type_decl<<"." << std::endl;
    }
}


Symbol ClassTable::find_lub(Symbol t1,Symbol t2){
    Symbol lub_type;
    std::list<Symbol> path = classtable->GetAllParents(t1);
    for (std::list<Symbol>::reverse_iterator iter = path.rbegin(); iter != path.rend(); ++iter){
        if (classtable->IsSubclass(*iter,t2)){
            lub_type = *iter;
            break;            
        }
    }
    return lub_type;
}


/*expression calculator for all type
assign to: chenrong
*/
Symbol assign_class::Type(){
    Symbol* lhs = attribtable.lookup(name);
    Symbol rhs = expr->Type();
    if (name == self){
        classtable->semant_error(curr_class->get_filename(),this) << "Cannot assign to 'self'." << std::endl;
        classtable->semant_error(curr_class->get_filename(),this) << "Type "<<rhs<<" of assigned expression does not conform to declared type SELF_TYPE of identifier self." << std::endl;
        type = curr_class->GetName();
        return type;
    }
    if (lhs == NULL) {
        classtable->semant_error(curr_class->get_filename(),this) << "Assignment to undeclared variable "<< name <<"." << std::endl;
        type = Object;
        return type;
    }
    if (classtable->IsSubclass(*lhs, rhs) == false) {
        classtable->semant_error(curr_class->get_filename(),this) << "Type "<<rhs<<" of assigned expression does not conform to declared type "<<*lhs<<" of identifier "<<name<<"." << std::endl;
        type = Object;
        return type;
    }
    type = rhs;
    return type;
}
Symbol static_dispatch_class::Type(){
    Symbol e0_type = expr->Type();
    if(!classtable->IsSubclass(type_name,e0_type)){
        classtable->semant_error(curr_class->get_filename(),this) <<"Expression type "<<e0_type<<" does not conform to declared static dispatch type "<<type_name<<"." << std::endl;
        type=Object;
        return type;
    }
    method_class* method = methodtables[type_name].lookup(name);
    if (method == NULL){
        classtable->semant_error(curr_class->get_filename(),this) << "Static dispatch to undefined method "<< name <<"." << std::endl;
        type = Object;
        return type;
    }
    for (int i = actual->first(); actual->more(i); i = actual->next(i)){
        Symbol actual_type = actual->nth(i)->Type();
        Symbol formal_type = method->GetFormals()->nth(i)->GetType();
        if (method != NULL){
            Symbol Symbol_type = method->GetFormals()->nth(i)->GetType();
            if (!classtable->IsSubclass(formal_type, actual_type)){
                classtable->semant_error(curr_class->get_filename(),this) << "In call of method "<<name<<", type "<<actual_type<<" of parameter "<<method->GetFormals()->nth(i)->GetName()<<" does not conform to declared type "<<formal_type<<"."<< std::endl;
            return type;
           }
        }
    }
    return type;
}
Symbol dispatch_class::Type(){
    Symbol e0_type = expr->Type();
    method_class* method = methodtables[e0_type].lookup(name);
    std::list<Symbol> path = classtable->GetAllParents(e0_type);
    for (std::list<Symbol>::iterator iter = path.begin(); iter != path.end(); ++iter){
        if ((method = methodtables[*iter].lookup(name)) != NULL){
            type=method->GetType();
            break;
        }
    }
    if (method == NULL){
        classtable->semant_error(curr_class->get_filename(),this) << "Dispatch to undefined method "<< name <<"." << std::endl;
        type = Object;
        return type;
    }
    for (int i = actual->first(); actual->more(i); i = actual->next(i)){
        Symbol actual_type = actual->nth(i)->Type();
        Symbol formal_type = method->GetFormals()->nth(i)->GetType();
        if (method != NULL){
            Symbol Symbol_type = method->GetFormals()->nth(i)->GetType();
            if (!classtable->IsSubclass(formal_type, actual_type)){
                classtable->semant_error(curr_class->get_filename(),this) << "In call of method "<<name<<", type "<<actual_type<<" of parameter "<<method->GetFormals()->nth(i)->GetName()<<" does not conform to declared type "<<formal_type<<"."<< std::endl;
            return type;
           }
        }
    }
    return type;
}
Symbol cond_class::Type(){
    Symbol cond = pred->Type();
    if(cond != Bool){
        classtable->semant_error(curr_class->get_filename(),this) << "If condition does not have type Bool." << std::endl;
    }
    Symbol e1_type = then_exp->Type();
    Symbol e2_type = else_exp->Type();
    type = classtable->find_lub(e1_type,e2_type);
    return type;
}
Symbol loop_class::Type(){
    Symbol cond = pred->Type();
    if(cond != Bool){
        classtable->semant_error(curr_class->get_filename(),this) << "Loop condition does not have type Bool." << std::endl;
    }
    body->Type();
    type = Object;
    return type;
}
Symbol typcase_class::Type(){
    Symbol expr_type = expr->Type();
    Case branch;
    std::list<Symbol> bran_types;
    std::list<Symbol> bran_type_decl;
    bool error=false;
    for (int i=cases->first(); cases->more(i); i = cases->next(i)){
        branch = cases->nth(i);
        Symbol case_type = ((branch_class *)branch)->GetExpr()->Type();
        Symbol case_type_dec = ((branch_class *)branch)->GetTypeDecl();
        for (std::list<Symbol>::reverse_iterator i = bran_type_decl.rbegin(); i!=bran_type_decl.rend(); ++i) {
        std::list<Symbol>::reverse_iterator j = i;
        j++;
        for (; j !=bran_type_decl.rend(); ++j) {
            if (*i == *j) {
                error=true;
                classtable->semant_error(curr_class->get_filename(),branch) << "Duplicate branch Int in case statement." << std::endl;
            }
        }
    }
        if(!error){
            bran_types.push_front(case_type);
            bran_type_decl.push_front(case_type_dec);
        }
        error=false;
    }


    type = *(bran_types.begin());
    for (std::list<Symbol>::iterator i = bran_types.begin(); i!=bran_types.end(); ++i) {
        type = classtable->find_lub(type, *i);
    }
    return type;
}

Symbol block_class::Type(){
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        type = body->nth(i)->Type();
    }
    return type;
}
Symbol let_class::Type(){
    attribtable.enterscope();

    Symbol init_type = init->Type();
    

    if (init_type != No_type) {
        if (classtable->IsSubclass(type_decl, init_type) == false) {
            classtable->semant_error(curr_class->get_filename(),this) << "Inferred type "<<init_type<<" of initialization of "<<identifier<<" does not conform to identifier's declared type "<<type_decl<<"." << std::endl;
        }
    }

    if (identifier == self) {
        classtable->semant_error(curr_class->get_filename(),this) << "'self' cannot be bound in a 'let' expression." << std::endl;
    }
    else
    {
        attribtable.addid(identifier, new Symbol(type_decl));
    }
    

    type = body->Type();
    attribtable.exitscope();
    return type;
}
Symbol plus_class::Type(){
    Symbol e1_type =e1->Type();
    Symbol e2_type =e2->Type();
    if(e1_type!=Int || e2_type!=Int){
        type=Int;
        classtable->semant_error(curr_class->get_filename(),this)<<"non-Int arguments: "<<e1_type<<" + "<<e2_type<<std::endl;
    }
    else  type=Int;
    return type;
}
Symbol sub_class::Type(){
    Symbol e1_type =e1->Type();
    Symbol e2_type =e2->Type();
    if(e1_type!=Int || e2_type!=Int){
        type=Int;
        classtable->semant_error(curr_class->get_filename(),this)<<"non-Int arguments: "<<e1_type<<" - "<<e2_type<<std::endl;
    }
    else  type=Int;
    return type;
}
Symbol mul_class::Type(){
    Symbol e1_type =e1->Type();
    Symbol e2_type =e2->Type();
    if(e1_type!=Int || e2_type!=Int){
        type = Int;
        classtable->semant_error(curr_class->get_filename(),this)<<"non-Int arguments: "<<e1_type<<" * "<<e2_type<<std::endl;
    }
    else  type = Int;
    return type;
}
Symbol divide_class::Type(){
    Symbol e1_type =e1->Type();
    Symbol e2_type =e2->Type();
    if(e1_type!=Int || e2_type!=Int){
        type = Int;
        classtable->semant_error(curr_class->get_filename(),this)<<"non-Int arguments: "<<e1_type<<" / "<<e2_type<<std::endl;
    }
    else  type = Int;
    return type;
}
Symbol neg_class::Type(){
    Symbol e1_type =e1->Type();
    if(e1_type!=Int){
        type=Int;
        classtable->semant_error(curr_class->get_filename(),this)<<"Argument of '~' has type "<<e1_type<<" instead of Int." <<std::endl;
    }
    else  type = Int;
    return type;
}
Symbol lt_class::Type(){
    Symbol e1_type =e1->Type();
    Symbol e2_type =e2->Type();
    if(e1_type!=Int || e2_type!=Int){
        type=Bool;
        classtable->semant_error(curr_class->get_filename(),this)<<"non-Int arguments: "<<e1_type<<" < "<<e2_type<<std::endl;
    }
    else  type = Bool;
    return type;
}
Symbol eq_class::Type(){
    Symbol e1_type = e1->Type();
    Symbol e2_type = e2->Type();
    if (e1_type == Int || e2_type == Int || e1_type == Bool || e2_type == Bool || e1_type == Str || e2_type == Str) {
        if (e1_type != e2_type) {
            classtable->semant_error(curr_class->get_filename(),this) << "Illegal comparison with a basic type." << std::endl;
            type = Bool;
        } else {
            type = Bool;
        }
    } else {
        type = Bool;
    }
    return type;
}
Symbol leq_class::Type(){
    Symbol e1_type = e1->Type();
    Symbol e2_type = e2->Type();
    if(e1_type!=Int || e2_type!=Int){
        type=Bool;
        classtable->semant_error(curr_class->get_filename(),this)<<"non-Int arguments: "<<e1_type<<" <= "<<e2_type<<std::endl;
    }
    else  type = Bool;
    return type;
}
Symbol comp_class::Type(){
    Symbol e1_type = e1->Type();
    if(e1_type!=Bool){
        type = Bool;
        classtable->semant_error(curr_class->get_filename(),this)<<"Argument of 'not' has type "<<e1_type<<" instead of Bool."<<std::endl;
    }
    else  type = Bool;
    return type;
}
Symbol int_const_class::Type(){type = Int; return type;}
Symbol bool_const_class::Type(){type = Bool; return type;}
Symbol string_const_class::Type(){type = Str; return type;}
Symbol new__class::Type(){
    if (type_name != SELF_TYPE && classtable->m_classes.find(type_name) == classtable->m_classes.end()) {
        type = Object;
        classtable->semant_error(curr_class->get_filename(),this) << "'new' used with undefined class "<< type_name <<"." << std::endl;
    }
    type = type_name;
    return type;
}
Symbol isvoid_class::Type(){
    e1->Type();
    type = Bool;
    return type;
}
Symbol no_expr_class::Type(){return No_type;}
Symbol object_class::Type(){
    if (name == self) {
        type = SELF_TYPE;
        return type;
    }

    Symbol* id_type = attribtable.lookup(name);
    if (id_type == NULL) {
        type = Object;
        classtable->semant_error(curr_class->get_filename(),this) << "Undeclared identifier "<<name<<"." << std::endl;
    } else {
        type = *id_type;
    }
    
    return type;
}