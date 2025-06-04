; SMT-LIB script to check the injectivity of the given
; C algorithm's state transition and output.
;
; The script sets up two distinct initial states, 'a' and 'b',
; for the variables mix, loopMix, and fast_loop.
; It then applies the state transition function and output calculation to both.
; Finally, it asserts that the resulting outputs are identical AND
; the resulting next internal states (mix, loopMix, fast_loop) are identical.
;
; - If the solver returns 'unsat' (Unsatisfiable), it has proven that
;   no such counterexample exists. This means the function IS INJECTIVE
;   (considering both the returned output and the next internal state).
;
; - If the solver returns 'sat' (Satisfiable), it has found a pair of
;   distinct initial states that collide into the same output AND
;   the same next internal state. This means the function IS NOT INJECTIVE.
;   The (get-model) command will show the counterexample.

; Set logic to Quantifier-Free Bit-Vectors
(set-logic QF_BV)

; Define the bit-vector size for uint64_t
(define-sort Bv64 () (_ BitVec 64))

; Define the constant for addition to fast_loop
(define-const const_99 Bv64 #x9999999999999999)

; --- Declare initial state variables for two instances 'a' and 'b' ---
; These are the inputs to one iteration of your algorithm.
(declare-const mix_a_curr Bv64)
(declare-const loopMix_a_curr Bv64)
(declare-const fast_loop_a_curr Bv64)

(declare-const mix_b_curr Bv64)
(declare-const loopMix_b_curr Bv64)
(declare-const fast_loop_b_curr Bv64)

; --- Assert that the initial full states 'a' and 'b' are distinct ---
(assert (or (distinct mix_a_curr mix_b_curr)
            (distinct loopMix_a_curr loopMix_b_curr)
            (distinct fast_loop_a_curr fast_loop_b_curr)))

; --- Apply the algorithm for instance 'a' ---

; uint64_t output_a = mix_a_curr + loopMix_a_curr;
(define-const output_a_val Bv64 (bvadd mix_a_curr loopMix_a_curr))

; uint64_t oldLoopMix_a = loopMix_a_curr; (used directly below)

; loopMix_a_next = fast_loop_a_curr ^ mix_a_curr;
(define-const loopMix_a_next Bv64 (bvxor fast_loop_a_curr mix_a_curr))

; mix_a_next = rotateLeft(mix_a_curr, 16) + rotateLeft(oldLoopMix_a, 40);
; (oldLoopMix_a is loopMix_a_curr)
(define-const mix_a_next Bv64 (bvadd ((_ rotate_left 16) mix_a_curr)
                                     ((_ rotate_left 40) loopMix_a_curr)))

; fast_loop_a_next = fast_loop_a_curr + 0x9999999999999999;
(define-const fast_loop_a_next Bv64 (bvadd fast_loop_a_curr const_99))

; --- Apply the algorithm for instance 'b' ---

; uint64_t output_b = mix_b_curr + loopMix_b_curr;
(define-const output_b_val Bv64 (bvadd mix_b_curr loopMix_b_curr))

; uint64_t oldLoopMix_b = loopMix_b_curr; (used directly below)

; loopMix_b_next = fast_loop_b_curr ^ mix_b_curr;
(define-const loopMix_b_next Bv64 (bvxor fast_loop_b_curr mix_b_curr))

; mix_b_next = rotateLeft(mix_b_curr, 16) + rotateLeft(oldLoopMix_b, 40);
; (oldLoopMix_b is loopMix_b_curr)
(define-const mix_b_next Bv64 (bvadd ((_ rotate_left 16) mix_b_curr)
                                     ((_ rotate_left 40) loopMix_b_curr)))

; fast_loop_b_next = fast_loop_b_curr + 0x9999999999999999;
(define-const fast_loop_b_next Bv64 (bvadd fast_loop_b_curr const_99))


; --- Assert that the returned output values are identical AND ---
; --- the resulting next internal states are identical ---
; This is the collision we are checking for.
(assert (and (= output_a_val output_b_val)         ; Returned output collision
             (= loopMix_a_next loopMix_b_next)     ; Next state variable collision
             (= mix_a_next mix_b_next)             ; Next state variable collision
             (= fast_loop_a_next fast_loop_b_next) ; Next state variable collision
))

; --- Check for satisfiability ---
(check-sat)
(get-model)
