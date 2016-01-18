;; Emacs integration.

(require 'cl)

;; Configuration.
(defvar mymake-command "mm" "Command-line used to run mymake.")
(defvar mymake-config "buildconfig" "Config file to look for when compiling.")
(defvar mymake-compile-frame nil "Use a new frame when compiling.")
(defvar mymake-compilation-w 100 "Compilation window width")
(defvar mymake-compilation-h 83 "Compilation window height")
(defvar mymake-compilation-adjust 10 "Compilation window adjustment")

;; Keybindings
(global-set-key (kbd "M-p") 'mymake-compile)
(global-set-key (kbd "C-c C-m") 'mymake-clean)
(global-set-key (kbd "C-c C-k") 'mymake-kill)
(global-set-key (kbd "C-c C-r") 'mymake-release)
(global-set-key (kbd "C-c q") 'mymake-command)

;; Error navigation.
(global-set-key (kbd "M-n") 'next-error)

;; Convenient delete/rename of files.
(global-set-key (kbd "C-c C-f C-r") 'mymake-rename-file)
(global-set-key (kbd "C-c C-f C-d") 'mymake-delete-file)

;; Bindable functions

(defun mymake-compile (force)
  "Compile using buildconfig (if it exists). Command line overridable by passing extra parameter."
  (interactive "P")
  (mymake-run :force force))

(defun mymake-clean ()
  "Clean project."
  (interactive)
  (mymake-run :prepend "-c"))

(defun mymake-release (force)
  "Compile in release mode."
  (interactive "P")
  (mymake-run :force force :replace "release"))

(defun mymake-release-64 (force)
  "Compile in release mode for x64."
  (interactive "P")
  (mymake-run :force force :replace "release 64"))

(defvar mymake-last-command "" "Last command used in 'mymake-command'.")

(defun mymake-command (command)
  "Run custom mymake command."
  (interactive
   (list (read-string "Command to mymake: " mymake-last-command)))
  (setq mymake-last-command command)
  (mymake-run :replace command))


(defun mymake-delete-file ()
  "Delete the file of the current buffer and deletes the buffer."
  (interactive)
  (if (yes-or-no-p (concat "Really delete " buffer-file-name "? "))
      (progn
	(delete-file buffer-file-name)
	(kill-buffer))))

(defun mymake-rename-file ()
  "Renames the file of the current buffer."
  (interactive)
  (let ((name (read-file-name "New name: ")))
    (rename-file buffer-file-name name)
    (set-visited-file-name name t t)))

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
	(file-name-as-directory dir)
      (let ((parent (parent-directory dir)))
	(if (endp parent)
	    'nil
	  (mymake-find-config parent))))))

(defun mymake-file-to-lines (file)
  (with-temp-buffer
    (insert-file-contents file)
    (split-string (buffer-string) "\n")))

(defun mymake-remove-comments (lines)
  (remove-if (lambda (x)
	       (or
		(= (length x) 0)
		(= (string-to-char x) ?#))) lines))

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
       (mymake-load-config-file (concat dir "buildconfig"))))))

(defun mymake-args-str (prepend replace config)
  (let ((replace-str (mymake-list-to-str replace))
	(prepend-str (mymake-list-to-str prepend)))
    (if (string= replace-str "")
	(mymake-list-to-str (mymake-remove-comments (list prepend-str config)))
      replace-str)))

(cl-defun mymake-run (&optional &key force &key prepend &key replace)
  (let* ((config (mymake-load-config))
	 (wd (first config))
	 (default-directory wd)
	 (args (mymake-args-str prepend replace (second config))))
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
	(if (not (frame-live-p mymake-compilation-frame))
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

