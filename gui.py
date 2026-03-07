import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image, ImageTk
import subprocess
import os

COMPILER_EXECUTABLE = "./parser"

class MiniCompilerGUI:

    def __init__(self, root):
        self.root = root
        self.root.title("Mini Compiler Frontend")
        self.root.geometry("1400x800")

        self.scale = 1.0
        self.original_image = None
        self.ast_photo = None

        self.create_widgets()

    def create_widgets(self):

        # =========================
        # Toolbar
        # =========================
        toolbar = tk.Frame(self.root)
        toolbar.pack(fill=tk.X)

        tk.Button(toolbar, text="Open", command=self.open_file).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Save", command=self.save_file).pack(side=tk.LEFT, padx=5)
        tk.Button(toolbar, text="Compile", command=self.compile_code).pack(side=tk.LEFT, padx=5)

        # =========================
        # Main Vertical Split
        # =========================
        main_pane = tk.PanedWindow(self.root, orient=tk.VERTICAL)
        main_pane.pack(fill=tk.BOTH, expand=True)

        # =========================
        # Top Horizontal Split
        # =========================
        top_pane = tk.PanedWindow(main_pane, orient=tk.HORIZONTAL)

        # Source Panel
        source_frame = tk.LabelFrame(top_pane, text="Source Code", width=350)
        self.source_text = tk.Text(source_frame, wrap="none")
        self.source_text.pack(fill=tk.BOTH, expand=True)
        top_pane.add(source_frame)

        # =========================
        # AST Panel (Interactive)
        # =========================
        ast_frame = tk.LabelFrame(top_pane, text="AST Viewer", width=700)

        # Zoom Buttons
        zoom_frame = tk.Frame(ast_frame)
        zoom_frame.pack(fill=tk.X)

        tk.Button(zoom_frame, text="Zoom In (+)", command=self.zoom_in).pack(side=tk.LEFT, padx=5)
        tk.Button(zoom_frame, text="Zoom Out (-)", command=self.zoom_out).pack(side=tk.LEFT)

        # Canvas + Scrollbars
        canvas_frame = tk.Frame(ast_frame)
        canvas_frame.pack(fill=tk.BOTH, expand=True)

        self.ast_canvas = tk.Canvas(canvas_frame, bg="white")
        self.ast_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        x_scroll = tk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL, command=self.ast_canvas.xview)
        x_scroll.pack(side=tk.BOTTOM, fill=tk.X)

        y_scroll = tk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=self.ast_canvas.yview)
        y_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.ast_canvas.configure(xscrollcommand=x_scroll.set, yscrollcommand=y_scroll.set)

        # Drag support
        self.ast_canvas.bind("<ButtonPress-1>", self.start_pan)
        self.ast_canvas.bind("<B1-Motion>", self.pan_image)

        # Mouse wheel zoom
        self.ast_canvas.bind("<MouseWheel>", self.mouse_zoom)  # Windows
        self.ast_canvas.bind("<Button-4>", self.mouse_zoom)    # Linux scroll up
        self.ast_canvas.bind("<Button-5>", self.mouse_zoom)    # Linux scroll down

        top_pane.add(ast_frame)

        # IR Panel
        ir_frame = tk.LabelFrame(top_pane, text="IR Code", width=350)
        self.ir_text = tk.Text(ir_frame, wrap="none")
        self.ir_text.pack(fill=tk.BOTH, expand=True)
        top_pane.add(ir_frame)

        main_pane.add(top_pane)

        # =========================
        # Bottom Output Panel
        # =========================
        bottom_frame = tk.LabelFrame(main_pane, text="Compiler Output", height=200)
        self.output_text = tk.Text(bottom_frame, wrap="none")
        self.output_text.pack(fill=tk.BOTH, expand=True)
        main_pane.add(bottom_frame)

        main_pane.paneconfigure(bottom_frame, height=200)

    # =========================
    # Zoom Functions
    # =========================

    def zoom_in(self):
        self.scale *= 1.2
        self.update_ast_image()

    def zoom_out(self):
        self.scale /= 1.2
        self.update_ast_image()

    def mouse_zoom(self, event):
        if event.num == 4 or event.delta > 0:
            self.zoom_in()
        elif event.num == 5 or event.delta < 0:
            self.zoom_out()

    def update_ast_image(self):
        if self.original_image:
            width = int(self.original_image.width * self.scale)
            height = int(self.original_image.height * self.scale)

            resized = self.original_image.resize((width, height), Image.LANCZOS)
            self.ast_photo = ImageTk.PhotoImage(resized)

            self.ast_canvas.delete("all")
            self.ast_canvas.create_image(0, 0, anchor="nw", image=self.ast_photo)
            self.ast_canvas.config(scrollregion=self.ast_canvas.bbox("all"))

    # =========================
    # Drag Support
    # =========================

    def start_pan(self, event):
        self.ast_canvas.scan_mark(event.x, event.y)

    def pan_image(self, event):
        self.ast_canvas.scan_dragto(event.x, event.y, gain=1)

    # =========================
    # File Handling
    # =========================

    def open_file(self):
        path = filedialog.askopenfilename(filetypes=[("C Files", "*.c"), ("All Files", "*.*")])
        if path:
            with open(path, "r") as f:
                self.source_text.delete("1.0", tk.END)
                self.source_text.insert(tk.END, f.read())

    def save_file(self):
        path = filedialog.asksaveasfilename(defaultextension=".c")
        if path:
            with open(path, "w") as f:
                f.write(self.source_text.get("1.0", tk.END))

    # =========================
    # Compile Function
    # =========================

    def compile_code(self):

        temp_file = "temp_input.c"
        with open(temp_file, "w") as f:
            f.write(self.source_text.get("1.0", tk.END))

        self.output_text.delete("1.0", tk.END)
        self.ir_text.delete("1.0", tk.END)

        result = subprocess.run(
            [COMPILER_EXECUTABLE, temp_file],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        self.output_text.insert(tk.END, result.stdout)

        if os.path.exists("ir.txt"):
            with open("ir.txt", "r") as f:
                self.ir_text.insert(tk.END, f.read())

        if os.path.exists("ast.dot"):
            subprocess.run(["dot", "-Tpng", "ast.dot", "-o", "ast.png"])

            if os.path.exists("ast.png"):
                self.original_image = Image.open("ast.png")
                self.scale = 1.0
                self.update_ast_image()


if __name__ == "__main__":
    root = tk.Tk()
    app = MiniCompilerGUI(root)
    root.mainloop()
