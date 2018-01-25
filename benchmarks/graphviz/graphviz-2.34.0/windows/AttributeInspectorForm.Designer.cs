namespace Graphviz
{
	partial class AttributeInspectorForm
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.attributePropertyGrid = new System.Windows.Forms.PropertyGrid();
			this.SuspendLayout();
			// 
			// attributePropertyGrid
			// 
			this.attributePropertyGrid.Dock = System.Windows.Forms.DockStyle.Fill;
			this.attributePropertyGrid.Location = new System.Drawing.Point(0, 0);
			this.attributePropertyGrid.Name = "attributePropertyGrid";
			this.attributePropertyGrid.PropertySort = System.Windows.Forms.PropertySort.NoSort;
			this.attributePropertyGrid.Size = new System.Drawing.Size(292, 266);
			this.attributePropertyGrid.TabIndex = 0;
			// 
			// AttributeInspectorForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(292, 266);
			this.Controls.Add(this.attributePropertyGrid);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
			this.Name = "AttributeInspectorForm";
			this.ShowInTaskbar = false;
			this.Text = "Attributes";
			this.TopMost = true;
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.PropertyGrid attributePropertyGrid;
	}
}