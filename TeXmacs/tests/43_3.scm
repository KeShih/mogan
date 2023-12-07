(define (export-as-latex-and-load path)
  (with path (string-append "$TEXMACS_PATH/tests/" path)
    (with tmpfile (url-temp)
      (load-buffer path)
      (buffer-export path tmpfile "latex")
      (string-load tmpfile))))
  
(define (load-latex path)
  (with path (string-append "$TEXMACS_PATH/tests/" path)
    (string-load path)))

(define (test-export-as-latex)
  (regression-test-group
   "export to latex and load as string" "load as string"
   export-as-latex-and-load load-latex
   (test "tm file with only a" "43_3_a.tm" "43_3_a.tex")
   (test "test empty tm file" "43_3_empty.tm" "43_3_empty.tex")))

(tm-define (test_43_3)
  (let ((n (+ (test-export-as-latex))))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of 9_1: ok\n")))
