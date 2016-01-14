;; Emacs integration.



;; Configuration.
(defvar mymake-command "mm" "Command-line used to run mymake.")
(defvar mymake-compile-frame nil "Use a new frame when compiling.")
(defvar mymake-compilation-w 100 "Compilation window width")
(defvar mymake-compilation-h 83 "Compilation window height")
(defvar mymake-compilation-adjust 30 "Compilation window adjustment")


;; Keybindings
(global-set-key (kbd "M-p") 'mymake-compile)
(global-set-key (kbd "C-m C-c") 'mymake-clean)
(global-set-key (kbd "C-m C-k") 'mymake-kill)

;; Implementations.


;; Compilation buffer management.

(setq mymake-compilation-window 'nil)
(setq mymake-compilation-frame 'nil)

(defun mymake-create-compilation-frame ()
  (let* ((current-params (frame-parameters))
	 (left-pos (cdr (assoc 'left current-params)))
	 (height (cdr (assoc 'height current-params)))
	 (created (make-frame '(inhibit-switch-frame))))
    (setq mymake-compilation-frame created)
    (setq mymake-compilation-window (frame-selected-window created))
    (set-frame-size created mymake-compilation-w mymake-compilation-h)
    (let ((created-w (frame-pixel-width created)))
      (set-frame-position (- left-pos created-w mymake-compilation-adjust) 0))))


(defun mymake-compilation-window ()
  (if (not mymake-compile-frame)
      'nil
    (progn
      (if (eq mymake-compilation-frame 'nil)
	  (mymake-create-compilation-frame)
	(if (not (frame-live-p compilation-frame))
	    (mymake-create-compilation-frame)))
      (window--display-buffer buffer mymake-compilation-window 'frame)
      mymake-compilation-window)))

(add-to-list display-buffer-alist
	     '("\\*compilation\\*" .
	       ((display-buffer-reuse-window
		 mymake-compilation-window
		 display-buffer-use-some-window
		 display-buffer-pop-up-window)
		. ((reusable-frames . t)
		   (inhibit-switch-frame . t)))))
