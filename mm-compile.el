;; Emacs integration.
;; Use M-p to compile using mymake. If a parent directory (or the current directory)
;; contains a file named 'buildconfig', that directory is used as a current directory
;; when executing mymake. If 'buildconfig' was found, its contents (except any commented
;; lines, using #) are used as a parameter to mymake.
;; If the 'buildconfig'-file is not found, emacs starts mymake in the buffer's directory,
;; using the current buffer name as a last-resort input. This is to make it super-simple
;; to start compiling using mymake; create a .cpp-file, write some code and hit M-p.
;; If you are annoyed by this, set 'mymake-no-default-input' to 't'.
;; To force a recompile, you can prefix any mymake-command with C-u.
;; Release: C-c C-r. This will run mymake with 'release' as parameter.
;; Clean: C-c C-m.
;; Custom command: C-c q
;;
;; Buildconfig syntax:
;; Lines starting with # are comments
;; Lines starting with : are directives to this emacs mode
;; All other lines are concatenated to form a command-line when running mymake.
;;
;; Directives are as follows:
;; :template yes     - add a template to cpp/h-files:
;; :pch foo.h        - include the precompiled header in cpp-files in templates
;; :namespace yes    - add "namespace <x> { }" to files, where <x> is the subdirectory the
;;                     current file is in with regards to the buildconfig file.
;; :template-headers - regex matching headers to apply template to.
;; :template-sources - regex matching source files to apply template to.

(require 'cl)

;; Configuration.
(defvar mymake-command "mm" "Command-line used to run mymake.")
(defvar mymake-config "buildconfig" "Config file to look for when compiling.")
(defvar mymake-compile-frame nil "Use a new frame when compiling.")
(defvar mymake-compilation-w 100 "Compilation window width")
(defvar mymake-compilation-h 83 "Compilation window height")
(defvar mymake-compilation-adjust-x 10 "Compilation window adjustment")
(defvar mymake-compilation-adjust-y 0 "Compilation window adjustment")
(defvar mymake-compilation-font-height 'nil "Compilation window font height")
(defvar mymake-no-default-input nil "Do not try to compile the current buffer if no buildconfig file is found.")

(defvar mymake-default-headers "\\(\\.h\\|\\.hh\\|\\.hpp\\)$" "Default regex for matching header files.")
(defvar mymake-default-sources "\\(\\.cpp\\|\\.cc\\|\\.cxx\\)$" "Default regex for matching source files.")

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

(defvar mymake-last-command "" "Last command used in the function 'mymake-command'.")

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
	(if (file-exists-p buffer-file-name)
	    (delete-file buffer-file-name))
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
    (cond ((eq nil r) 'nil)
	  ((< (length r) (length dir)) r)
	  (t 'nil))))

;; Find a directory containing a buildconfig file. Returns nil if none found.
(defun mymake-find-config (dir)
  (let ((file (concat (file-name-as-directory dir) "buildconfig")))
    (if (file-exists-p file)
	(file-name-as-directory dir)
      (let ((parent (parent-directory dir)))
	(if (eq nil parent)
	    'nil
	  (mymake-find-config parent))))))

(defun mymake-file-to-lines (file)
  (with-temp-buffer
    (insert-file-contents file)
    (split-string (buffer-string) "\n")))

(defun mymake-remove-comments (lines)
  "Removes any lines starting with #, : or empty lines"
  (remove-if (lambda (x)
	       (or
		(= (length x) 0)
		(= (string-to-char x) ?:)
		(= (string-to-char x) ?#)))
	     lines))

(defun mymake-list-to-str (lines)
  (cond ((stringp lines) lines)
	((consp lines) (reduce (lambda (a b) (concat a " " b)) lines))
	(t "")))

(defun mymake-option (line)
  "Extract a single option from a line, if it is a valid option."
  (cond ((string= line "") 'nil)
	((not (= (string-to-char line) ?:)) 'nil)
	(t
	 (let ((parts (split-string (substring line 1))))
	   (if (> (length parts) 1)
	       (cons (intern (first parts))
		     (mymake-list-to-str (rest parts)))
	     'nil)))))

(defun mymake-assoc (value options &optional default)
  "Find a configuration option."
  (let ((r (assoc value options)))
    (if (endp r)
	default
      (cdr r))))

(defun mymake-options (lines)
  "Find options (starting with : )"
  (if (endp lines)
      '()
    (let ((opt (mymake-option (first lines))))
      (if (eq nil opt)
	  (mymake-options (rest lines))
	(cons opt (mymake-options (rest lines)))))))

(defun mymake-load-config-file (file)
  "Returns an alist with the configuration"
  (let* ((file-lines (mymake-file-to-lines file))
	 (cmdline (mymake-list-to-str (mymake-remove-comments file-lines)))
	 (options (mymake-options file-lines)))

    (cons
     (cons 'cmdline cmdline)
     options)))

(defun mymake-load-config ()
  "Returns an alist with configuration + current directory as 'dir'"
  (let* ((buffer-dir (if (eq nil (buffer-file-name))
			 default-directory
		       (parent-directory (buffer-file-name))))
	 (dir (mymake-find-config buffer-dir)))
    (if (eq nil dir)
	;; No config file, we probably want to add the buffer file name as well.
	(list
	 (cons 'dir buffer-dir)
	 (cons 'cmdline
	       (if (or mymake-no-default-input (eq nil (buffer-file-name)))
		   ""
		 (concat "--default-input " (file-name-nondirectory (buffer-file-name))))))
      (cons
       (cons 'dir dir)
       (mymake-load-config-file (concat dir "buildconfig"))))))

(defun mymake-args-str (prepend replace config)
  (let ((replace-str (mymake-list-to-str replace))
	(prepend-str (mymake-list-to-str prepend)))
    (if (string= replace-str "")
	(mymake-list-to-str (mymake-remove-comments (list prepend-str config)))
      replace-str)))

(cl-defun mymake-run (&optional &key force &key prepend &key replace)
  (let* ((config (mymake-load-config))
	 (wd (mymake-assoc 'dir config))
	 (default-directory wd)
	 (args (mymake-args-str prepend replace (mymake-assoc 'cmdline config))))
    (compile (concat
	      mymake-command " "
	      (if (eq nil force)
		  args
		(concat "-f " args))) t)))

;; Add template if desired.
(add-hook 'find-file-hooks 'mymake-maybe-add-template)
(defun mymake-maybe-add-template ()
  "Check to see if we want to add templates to a newly-created file."
  (if (file-exists-p buffer-file-name)
      'nil
    (let ((config (mymake-load-config)))
      (cond ((not (string= (mymake-assoc 'template config "no") "yes")) 'nil)
	    ((string-match (mymake-assoc 'template-source config mymake-default-sources) buffer-file-name)
	     (mymake-add-source-template config))
	    ((string-match (mymake-assoc 'template-header config mymake-default-headers) buffer-file-name)
	     (mymake-add-header-template config))
	    (t 'nil)))))

(defun mymake-add-source-template (config)
  (let ((pch (mymake-assoc 'pch config 'nil)))
    ;; Syntax highlighting works _way_ better when one string is inserted instead of in different parts...
    (if (not (eq nil pch))
	(insert (concat "#include \"" pch "\"\n"))))
  (let* ((header (mymake-assoc 'header-ext config "h"))
	 (fn (concat (file-name-base buffer-file-name) "." header)))
    (insert (concat "#include \"" fn "\"\n")))

  (insert "\n")

  (mymake-insert-namespace config))

(defun mymake-add-header-template (config)
  (insert "#pragma once\n\n")
  (mymake-insert-namespace config))

(defun mymake-insert-namespace (config)
  (if (string= (mymake-assoc 'namespace config "no") "yes")
      (let ((name (mymake-target-name config)))
	(if name
	    (progn
	      (insert "namespace " (downcase name) " {\n\n")
	      (let ((pos (point)))
		(insert "\n\n}\n")
		(goto-char pos)
		(indent-for-tab-command)))))))

(defun mymake-target-name (config)
  (let* ((wd (downcase (mymake-assoc 'dir config)))
	 (left (downcase (substring buffer-file-name 0 (length wd))))
	 (right (downcase (substring buffer-file-name (length wd)))))
    (if (string= left wd)
	(let ((parts (split-string right "/")))
	  (if (> (length parts) 1)
	      (first parts)
	    'nil))
      'nil)))


;; Compilation buffer management.

(setq mymake-compilation-window 'nil)
(setq mymake-compilation-frame 'nil)

(defun mymake-create-compilation-frame ()
  (let* ((current-params (frame-parameters))
	 (old-frame (selected-frame))
	 (left-pos (cdr (assoc 'left current-params)))
	 (top-pos (cdr (assoc 'top current-params)))
	 (height (cdr (assoc 'height current-params)))
	 (created (make-frame (list (cons 'width mymake-compilation-w)
				    (cons 'height mymake-compilation-h)))))
    (if (numberp mymake-compilation-font-height)
	(set-face-attribute 'default created :height mymake-compilation-font-height))
    (setq mymake-compilation-frame created)
    (setq mymake-compilation-window (frame-selected-window created))
    (let ((created-w (frame-pixel-width created)))
      (set-frame-position created
			  (- left-pos created-w mymake-compilation-adjust-x)
			  (+ top-pos mymake-compilation-adjust-y)))
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

