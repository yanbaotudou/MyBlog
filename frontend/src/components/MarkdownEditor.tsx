import ReactMarkdown from "react-markdown";

interface MarkdownEditorProps {
  title: string;
  setTitle: (value: string) => void;
  content: string;
  setContent: (value: string) => void;
}

export function MarkdownEditor({ title, setTitle, content, setContent }: MarkdownEditorProps) {
  return (
    <div className="editor-layout">
      <section className="editor-panel">
        <label htmlFor="title">标题</label>
        <input
          id="title"
          value={title}
          onChange={(e) => setTitle(e.target.value)}
          placeholder="输入文章标题"
          maxLength={120}
        />

        <label htmlFor="content">正文（Markdown）</label>
        <textarea
          id="content"
          value={content}
          onChange={(e) => setContent(e.target.value)}
          placeholder="记录今天学到的内容..."
          rows={16}
        />
      </section>

      <section className="preview-panel">
        <h3>预览</h3>
        <div className="markdown-body">
          <ReactMarkdown>{content || "_暂无内容_"}</ReactMarkdown>
        </div>
      </section>
    </div>
  );
}
