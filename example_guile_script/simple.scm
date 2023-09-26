;; Called from Bren
(define (bren-bridge filepath)
  ;; It seems that telling Guile to use the current locale
  ;; is necessary in order to get scandinavian characters
  ;; to work in filepath
  (setlocale LC_ALL "")
  (print-filepath filepath))

(define (print-filepath filepath)
  (display "Hello from Guile")
  (newline)
  (display filepath)
  (newline))
