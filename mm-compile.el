;; Emacs integration.

;; Configuration.
(defvar mymake-command "mm" "Command-line used to run mymake.")
(defvar mymake-config "buildconfig" "Config file to look for when compiling.")
(defvar mymake-compile-frame nil "Use a new frame when compiling.")
(defvar mymake-compilation-w 100 "Compilation window width")
(defvar mymake-compilation-h 83 "Compilation window height")
(defvar mymake-compilation-adjust 30 "Compilation window adjustment")


;; Keybindings
(global-set-key (kbd "M-p") 'mymake-compile)
(global-set-key (kbd "C-c C-m") 'mymake-clean)
(global-set-key (kbd "C-c C-k") 'mymake-kill)

;; Bindable functions

;; Compile using buildconfig (if it exists). Command line overridable by passing extra parameter.
(defun mymake-compile (force)
  (interactive "P")
  (mymake-run force 'nil 'nil))

;; Compile in release mode.
(defun mymake-release (force)
  (interactive "P")
  (mymake-run force 'nil "release"))

;; Compile in release mode for x64.
(defun mymake-release-64 (force)
  (interactive "P")
  (mymake-run force 'nil '("release" "64")))

;; Implementations.

;; Get parent directory or nil if none.
(defun parent-directory (dir)
  (let ((r (file-name-directory (directory-file-name dir))))
    (cond ((endp r) 'nil)
	  ((< (length r) (length dir)) r)
	  (t 'nil))))

;; Find a directory containing a buildconfig file. Returns nil if none found.
(defun mymake-find-config (dir)
  (let ((file (concat (file-name-as-directory dir) "buildconfig")))
    (if (file-exists-p file)
	file
      (let ((parent (parent-directory dir)))
	(if (endp parent)
	    'nil
	  (mymake-find-config parent))))))

(defun mymake-file-to-lines (file)
  (with-temp-buffer
    (insert-file-contents file)
    (split-string (buffer-string) "\n")))

(defun mymake-remove-comments (lines)
  (remove-if (lambda (x) (= (string-to-char x) ?#)) lines))

(defun mymake-list-to-str (lines)
  (cond ((stringp lines) lines)
	((consp lines) (reduce (lambda (a b) (concat a " " b)) lines))
	(t "")))

(mymake-list-to-str '())

(defun mymake-load-config-file (file)
  (let ((lines (mymake-remove-comments (mymake-file-to-lines file))))
    (mymake-list-to-str lines)))

(defun mymake-load-config ()
  (let ((dir (mymake-find-config (buffer-file-name))))
    (if (endp dir)
	(list
	 (parent-directory (buffer-file-name))
	 "")
      (list
       dir
       (mymake-load-config-file (concat (file-name-as-directory dir) "buildconfig"))))))

(defun mymake-run (&optional force prepend replace)
  (let* ((config (mymake-load-config))
	 (wd (first config))
	 (default-directory wd)
	 (args (cond ((stringp replace) replace)
		     ((listp replace) (mymake-list-to-str replace))
		     (t (concat (mymake-list-to-str prepend) " "
				(second config))))))
    (compile (concat
	      mymake-command " "
	      (if (endp force)
		  args
		(concat "-f " args))))))


;; Compilation buffer management.

(setq mymake-compilation-window 'nil)
(setq mymake-compilation-frame 'nil)

(defun mymake-create-compilation-frame ()
  (let* ((current-params (frame-parameters))
	 (old-frame (selected-frame))
	 (left-pos (cdr (assoc 'left current-params)))
	 (height (cdr (assoc 'height current-params)))
	 (created (make-frame (list (cons 'width mymake-compilation-w)
				    (cons 'height mymake-compilation-h)))))
    (setq mymake-compilation-frame created)
    (setq mymake-compilation-window (frame-selected-window created))
    (let ((created-w (frame-pixel-width created)))
      (set-frame-position created (- left-pos created-w mymake-compilation-adjust) 0))
    (select-frame-set-input-focus old-frame)))


(defun mymake-compilation-window (buffer alist)
  (if (not mymake-compile-frame)
      'nil
    (progn
      (if (eq mymake-compilation-frame 'nil)
	  (mymake-create-compilation-frame)
	(if (not (frame-live-p compilation-frame))
	    (mymake-create-compilation-frame)))
      (window--display-buffer buffer mymake-compilation-window 'frame alist)
      mymake-compilation-window)))

(add-to-list 'display-buffer-alist
	     '("\\*compilation\\*" .
	       ((display-buffer-reuse-window
		 mymake-compilation-window
		 display-buffer-use-some-window
		 display-buffer-pop-up-window)
		. ((reusable-frames . t)
		   (inhibit-switch-frame . t)
		   (inhibit-same-window . t)))))

