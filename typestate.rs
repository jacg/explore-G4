use core::marker::PhantomData;

// The status of any component could be in three different states.
use private::State;
pub enum ToDo    {} // Needs to be set
pub enum Allowed {} // Setting possible but not necessary
pub enum Done    {} // Cannot be set

// Wrap the State trait in a private submodule, to prevent clients from adding
// meaningless implementations: the sealed trait pattern
mod private {
    pub trait State {}
    impl State for super::ToDo    {}
    impl State for super::Allowed {}
    impl State for super::Done    {}
}

// The values that need to be collected by the setters
struct Data { a: usize, b: usize, c: usize }
impl Data { pub fn new() -> Self { Self { a:0, b:0, c:0 }}}

// The builder object: `Build`, type-parametrized on the configuration state of
// each component. We start off with all three components is the `ToDo` state.
// In general it would be reasonable for some of these to be `Allowed`.
pub struct Build<A: State=ToDo, B: State=ToDo, C: State=ToDo> {
    data: Box<Data>,
    // If we don't use the parameter types at all, the compiler will complain.
    // So we include them in the struct as PhantomData. It's actually impossible
    // to create values of the State types: they are zero-variant enums. Their
    // only purpose is to allow us to create different versions of `Build` by
    // using different parameter types.
    p: core::marker::PhantomData<(A, B, C)>,
}

// Convenient entry point
fn build() -> Build { Build { data: Box::new(Data::new()), p: PhantomData } }

// Regardless of the states of `B` and `C`, if `A=ToDo` then `aa` is
// implemented. Calling `aa` changes `A` to `Done`. As `aa` is not implemented
// elsewhere, we will not be allowed to call it again.
impl<B: State, C: State> Build<ToDo, B, C> {
    pub fn aa(mut self, a: usize) -> Build<Done, B, C> {
        self.data.a = a;
        Build { data: self.data, p: PhantomData }
    }
}

// Similar to `aa`: `bb` is implemented iff `B=ToDo`; calling `bb` will change
// `B` to `Done`; the `A` and `C` states are irrelevant.
impl<A: State, C: State> Build<A, ToDo, C> {
    pub fn bb(mut self, b: usize) -> Build<A, Done, C> {
        self.data.b = b;
        Build { data: self.data, p: PhantomData }
    }
}

// Similar to `aa` and `bb` but `cc` changes the `C` state to `Allowed` rather
// than `Done`, and ...
impl<A: State, B: State> Build<A, B, ToDo> {
    pub fn cc(mut self, c: usize) -> Build<A, B, Allowed> {
        self.data.c = c;
        Build { data: self.data, p: PhantomData }
    }
}

// ... `cc` is also implemented in the `C=Allowed` state, so we can call it
// again. Here it preserves `C` as `Allowed` so we'll be able to call it as many
// times as we want.
impl<A: State, B: State> Build<A, B, Allowed> {
    pub fn cc(mut self, c: usize) -> Self {
        self.data.c += c;
        self
    }
}

// The ready state: the only state in which we can call `go`.
impl Build<Done, Done, Allowed> {
    pub fn go(&self) { let d = &self.data; println!("{} {} {}", d.a, d.b, d.c); }
}

// Crucially we have 2 x 2 x 3 (2 for A and B, but 3 for C) = 12 states, but we
// only needed to write 5 implementations of `Build`, because in this approach
// 'parallel' functions are implemented polymorphically with a single
// definition.

fn main() {
    build().aa(1).bb(2).cc(3).go();
    build().aa(1).cc(2).bb(3).go();
    build().bb(1).aa(2).cc(3).go();
    build().cc(1).aa(2).bb(3).go();
    build().bb(1).cc(2).aa(3).go();
    build().cc(1).bb(2).aa(3).go();

    // cc is allowed multiple times
    build().cc(1).bb(2).aa(3).cc(4).go();

    // Compile-time errors: something left unspecified

    // build.go();
    // build.aa(1).go();
    // build.bb(1).go();
    // build.cc(1).go();
    // build().aa(1).bb(2).go();
    // build().aa(1).cc(2).go();
    // build().bb(1).aa(2).go();
    // build().cc(1).aa(2).go();
    // build().cc(1).bb(2).go();

    // Compile-time errors: multiple specification

    // build().aa(1).aa(2);
    // build().aa(1).bb(2).aa(3);
    // etc.

}
