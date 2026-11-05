-- Custom pandoc reader: reads trab_final_metpesq.md as it is written today and
-- reshapes it for the abnTeX2 template. The .md itself is never modified.

local CITACOES = {
  ["1"] = "bryant2011", ["2"] = "caini2024entt", ["3"] = "romeo2024ecst",
  ["6"] = "driesen1996", ["7"] = "linietsky2021", ["8"] = "caini2019",
  ["9"] = "mertensflecs", ["10"] = "mertens2022", ["11"] = "godotnode",
}

-- Images drawn smaller than the default width (fraction of that width).
local ESCALAS = { ["images/matrix2.png"] = 0.5 }

local PADRAO_CITE = "<cite>\\%[%[(%d+)%]\\%]%[%d+%]</cite>?"

local function chave(n)
  return CITACOES[n] or error("citation [" .. n .. "] has no entry in CITACOES")
end

-- <cite>\[[7]\][7]</cite> => [@key]. ABNT puts the citation before the final period.
local function converter_citacoes(texto)
  texto = texto:gsub("%.%s*" .. PADRAO_CITE, function(n)
    return " [@" .. chave(n) .. "]."
  end)
  texto = texto:gsub("%s*" .. PADRAO_CITE, function(n)
    return " [@" .. chave(n) .. "]"
  end)
  return texto
end

local function latex(inlines)
  local saida = pandoc.write(pandoc.Pandoc({ pandoc.Plain(inlines) }), "latex")
  return (saida:gsub("%s+$", ""))
end

-- For an italic "Figure 3: text" paragraph, returns the inlines after the label.
local function legenda(bloco, rotulo)
  if not bloco or bloco.t ~= "Para" or #bloco.content ~= 1 then return nil end
  local enfase = bloco.content[1]
  if enfase.t ~= "Emph" then return nil end
  local c = enfase.content
  if #c < 5 or c[1].t ~= "Str" or c[1].text ~= rotulo then return nil end
  if not (c[3].t == "Str" and c[3].text:match("^%d+:$")) then return nil end
  local resto = pandoc.Inlines({})
  for i = 5, #c do resto:insert(c[i]) end
  return resto
end

-- An image with its caption on the very next line (no blank line between them)
-- reaches us as one paragraph: Image, SoftBreak, Emph. Returns the caption's inlines.
local function legenda_colada(bloco)
  local c = bloco.content
  if #c ~= 3 or c[1].t ~= "Image" or c[2].t ~= "SoftBreak" then return nil end
  local paragrafo = pandoc.Para({ c[3] })
  return legenda(paragrafo, "Figure") or legenda(paragrafo, "Matrix")
end

local function fonte_de(caminho, meta)
  for prefixo, fonte in pairs(meta.fontes or {}) do
    if caminho:sub(1, #prefixo) == prefixo then
      return pandoc.utils.stringify(fonte)
    end
  end
  return nil
end

-- ABNT: title above the figure, source below it.
local function figura(fig, inlines, meta)
  local imagem
  pandoc.walk_block(fig, { Image = function(i) imagem = imagem or i end })
  local fonte = fonte_de(imagem.src, meta)
  local largura = 0.85 * (ESCALAS[imagem.src] or 1)
  return pandoc.RawBlock("latex", table.concat({
    "\\begin{figure}[htbp]",
    "\\centering",
    "\\caption{" .. latex(inlines) .. "}",
    "\\includegraphics[width=" .. largura .. "\\textwidth,height=0.6\\textheight,keepaspectratio]{" .. imagem.src .. "}",
    fonte and ("\\fonte{" .. fonte .. "}") or "",
    "\\end{figure}",
  }, "\n"))
end

-- Code in listings because plain verbatim cannot wrap the long pseudocode lines.
local function codigo(bloco)
  local opcoes = bloco.classes:includes("cpp") and "[language=C++]" or ""
  return pandoc.RawBlock("latex",
    "\\begin{lstlisting}" .. opcoes .. "\n" .. bloco.text .. "\n\\end{lstlisting}")
end

local function tabela(tab, inlines)
  if inlines then
    tab.caption.long = pandoc.Blocks({ pandoc.Plain(inlines) })
  end
  if #tab.colspecs > 6 then
    -- The row labels need more room than the source's dashes give them;
    -- take it from the narrow "cdots" columns (same width as column 3).
    local estreita = tab.colspecs[3][2]
    for _, spec in ipairs(tab.colspecs) do
      if spec[2] == estreita then spec[2] = estreita - 0.012 end
    end
    tab.colspecs[1][2] = tab.colspecs[1][2] + 0.036
    return {
      pandoc.RawBlock("latex", "\\begin{landscape}\\begingroup\\scriptsize\\setlength{\\tabcolsep}{3pt}"),
      tab,
      pandoc.RawBlock("latex", "\\endgroup\\end{landscape}"),
    }
  end
  return { tab }
end

-- Number comes from LaTeX, level comes from the number ("3.1" => section).
local function titulo(h, vistos, meta, saida)
  local primeiro = h.content[1]
  local num = primeiro and primeiro.t == "Str" and primeiro.text:gsub("%.$", "")
  local conteudo = pandoc.Inlines(h.content)
  if num and num:match("^%d+$") then
    h.level = 1
    vistos[num] = true
  elseif num and num:match("^%d+%.%d+$") then
    h.level = 2
    local cap = num:match("^%d+")
    if not vistos[cap] then
      vistos[cap] = true
      local nome = (meta.capitulos or {})[cap]
      if not nome then error("no title for chapter " .. cap .. " in metadados.yaml") end
      saida:insert(pandoc.Header(1, pandoc.utils.stringify(nome)))
    end
  else
    num = nil
  end
  if num then
    conteudo:remove(1)
    if conteudo[1] and conteudo[1].t == "Space" then conteudo:remove(1) end
  end
  local ultimo = conteudo[#conteudo]
  if ultimo and ultimo.t == "Str" then
    ultimo.text = ultimo.text:gsub(":$", "")
  end
  h.content = conteudo
  saida:insert(h)
end

local function ler_metadados()
  local arquivo = assert(io.open("abnt/metadados.yaml"))
  local yaml = arquivo:read("a")
  arquivo:close()
  return pandoc.read("---\n" .. yaml .. "\n---\n", "markdown").meta
end

function Reader(entrada, opcoes)
  local texto = converter_citacoes(tostring(entrada))
  local doc = pandoc.read(texto,
    "markdown+lists_without_preceding_blankline", opcoes)
  local meta = ler_metadados()

  local blocos = doc.blocks
  local saida = pandoc.Blocks({})
  local resumo = pandoc.Blocks({})
  local vistos = {}
  local i = 1

  -- Draft banner, title and author lines: the template builds these from metadata.
  while blocos[i] and not (blocos[i].t == "Header"
      and pandoc.utils.stringify(blocos[i]) == "Abstract") do
    i = i + 1
  end
  i = i + 1
  while blocos[i] and blocos[i].t ~= "Header" do
    resumo:insert(blocos[i])
    i = i + 1
  end
  doc.meta.abstract = pandoc.MetaBlocks(resumo)

  while blocos[i] do
    local b = blocos[i]
    if b.t == "Header" and pandoc.utils.stringify(b) == "References" then
      saida:insert(pandoc.RawBlock("latex", "\\postextual"))
      saida:insert(pandoc.Header(1, "References", { class = "unnumbered" }))
      saida:insert(pandoc.RawBlock("latex", "\\begingroup\\SingleSpacing\\raggedright"))
      saida:insert(pandoc.Div({}, { id = "refs" }))
      saida:insert(pandoc.RawBlock("latex", "\\endgroup"))
      break -- the handwritten list is replaced by the citeproc one
    elseif b.t == "Header" then
      titulo(b, vistos, meta, saida)
    elseif b.t == "Figure" then
      local inlines = legenda(blocos[i + 1], "Figure")
      if inlines then i = i + 1 end
      saida:insert(figura(b, inlines or pandoc.Inlines({}), meta))
    elseif b.t == "Para" and legenda_colada(b) then
      saida:insert(figura(b, legenda_colada(b), meta))
    elseif b.t == "Table" then
      local inlines = legenda(blocos[i + 1], "Matrix") or legenda(blocos[i + 1], "Table")
      if inlines then i = i + 1 end
      saida:extend(tabela(b, inlines))
    elseif b.t == "CodeBlock" then
      saida:insert(codigo(b))
    else
      saida:insert(b)
    end
    i = i + 1
  end

  doc.blocks = saida
  return doc
end
